/**
 * @file real_sid_chip.cpp
 * @brief Implementation of RealSidChip: timed write queue, output task and 74HCT595/GPIO driver.
 */
#include "sidchip.h"
#if SID_ENGINE == SID_ENGINE_REAL
#include <Arduino.h>
#include <esp_timer.h>
#include <esp_rom_sys.h>
#include <soc/gpio_reg.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

static const uint8_t RESET_ITEM = 0xFF;   ///< Item::reg value that means "reset the chip".
static const int PHI2_LEDC_CHANNEL = 7;   ///< LEDC channel generating the PHI2 clock.
static const int64_t LATENCY_US = int64_t(REAL_SID_LATENCY_MS) * 1000;   ///< Write latency in microseconds.
static const int64_t MAX_LEAD_US = LATENCY_US + 150000;   ///< A write further in the future = clocks drifted apart, re-anchor.
static const int64_t IDLE_MUTE_US = 60000;                 ///< No emulation for this long (pause, stop) -> mute the chip.
static const int64_t MAX_SPIN_US = 50000;                  ///< Yield the CPU at least this often while busy-waiting (watchdog).

/**
 * @brief Sets a GPIO high directly through the GPIO registers (digitalWrite is too slow).
 * @param p GPIO number (0..39 / 0..48).
 */
static inline void pinHigh(int p) {
    if (p < 32) REG_WRITE(GPIO_OUT_W1TS_REG, 1UL << p);
    else REG_WRITE(GPIO_OUT1_W1TS_REG, 1UL << (p - 32));
}
/**
 * @brief Sets a GPIO low directly through the GPIO registers.
 * @param p GPIO number.
 */
static inline void pinLow(int p) {
    if (p < 32) REG_WRITE(GPIO_OUT_W1TC_REG, 1UL << p);
    else REG_WRITE(GPIO_OUT1_W1TC_REG, 1UL << (p - 32));
}

/**
 * @brief Starts (or retunes) the 50% duty PHI2 square wave on REAL_SID_PIN_PHI2 using LEDC.
 * @param hz Clock frequency in Hz (C64 PAL or NTSC clock).
 */
static void startPhi2(double hz) {
    // 1 bit rozdzielczosci = przebieg prostokatny 50%
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    static bool attached = false;
    if (!attached) attached = ledcAttachChannel(REAL_SID_PIN_PHI2, uint32_t(hz), 1, PHI2_LEDC_CHANNEL);
    else ledcChangeFrequency(REAL_SID_PIN_PHI2, uint32_t(hz), 1);
    ledcWrite(REAL_SID_PIN_PHI2, 1);
#else
    static bool attached = false;
    if (!attached) {
        ledcSetup(PHI2_LEDC_CHANNEL, hz, 1);
        ledcAttachPin(REAL_SID_PIN_PHI2, PHI2_LEDC_CHANNEL);
        attached = true;
    } else {
        ledcChangeFrequency(PHI2_LEDC_CHANNEL, hz, 1);
    }
    ledcWrite(PHI2_LEDC_CHANNEL, 1);
#endif
}

void RealSidChip::begin() {
    pinMode(REAL_SID_PIN_CS, OUTPUT);
    digitalWrite(REAL_SID_PIN_CS, HIGH);
    pinMode(REAL_SID_PIN_RES, OUTPUT);
    digitalWrite(REAL_SID_PIN_RES, LOW);
    pinMode(REAL_SID_PIN_DATA, OUTPUT);
    pinMode(REAL_SID_PIN_SHIFT, OUTPUT);
    pinMode(REAL_SID_PIN_LATCH, OUTPUT);
    digitalWrite(REAL_SID_PIN_SHIFT, LOW);
    digitalWrite(REAL_SID_PIN_LATCH, LOW);
    queue = xQueueCreate(REAL_SID_QUEUE_LEN, sizeof(Item));
    xTaskCreatePinnedToCore(taskEntry, "real-sid", 3072, this, REAL_SID_TASK_PRIORITY, nullptr,
                            REAL_SID_TASK_CORE);
}

void RealSidChip::init(double clockHz, double sampleRate, Sid::Model) {
    if (!queue) begin();
    shadow.init(clockHz, sampleRate, REAL_SID_MODEL ? Sid::MOS8580 : Sid::MOS6581);
    usPerCycle = 1000000.0 / clockHz;
    phiHz = clockHz;          // PHI2 = zegar PAL albo NTSC, zgodnie z plikiem
    timeCycles = 0;
    reset();
}

void RealSidChip::reset() {
    shadow.reset();
    if (!queue) return;
    // nowe "pokolenie": zadanie wyjsciowe pomija zapisy poprzedniego utworu
    gen = gen + 1;
    xQueueReset((QueueHandle_t)queue);
    Item it = {0, gen, RESET_ITEM, 0};
    xQueueSend((QueueHandle_t)queue, &it, portMAX_DELAY);
    anchored = false;
}

void RealSidChip::write(uint8_t reg, uint8_t value) {
    reg &= 0x1F;
    shadow.write(reg, value);
    if (!queue || reg > 0x18) return;
    int64_t now = esp_timer_get_time();
    int64_t v = int64_t(timeCycles * usPerCycle);
    int64_t t = anchorUs + v;
    // poczatek utworu, powrot z pauzy albo rozjechane zegary (I2S vs esp_timer): nowy punkt odniesienia
    if (!anchored || t - now < 2000 || t - now > MAX_LEAD_US) {
        anchorUs = now + LATENCY_US - v;
        anchored = true;
        t = now + LATENCY_US;
    }
    Item it = {t, gen, reg, value};
    xQueueSend((QueueHandle_t)queue, &it, pdMS_TO_TICKS(50));
}

float RealSidChip::clock() {
    clockCount = clockCount + 1;
    float s = shadow.clock();
    return REAL_SID_EMU_AUDIO ? s : 0.0f;
}

// ---------------------------------------------------------------- strona sprzetowa

// 16 bits into the two 74HCT595s: address first (it ends up in the second register), then data.
// Then a /CS pulse longer than the PHI2 period - the SID latches the register on the falling PHI2 edge.
void RealSidChip::hwWrite(uint8_t reg, uint8_t val) {
    uint16_t word = (uint16_t(reg & 0x1F) << 8) | val;
    for (int i = 15; i >= 0; i--) {
        if (word & (1 << i)) pinHigh(REAL_SID_PIN_DATA);
        else pinLow(REAL_SID_PIN_DATA);
        pinHigh(REAL_SID_PIN_SHIFT);
        pinLow(REAL_SID_PIN_SHIFT);
    }
    pinHigh(REAL_SID_PIN_LATCH);
    pinLow(REAL_SID_PIN_LATCH);
    pinLow(REAL_SID_PIN_CS);
    esp_rom_delay_us(2);
    pinHigh(REAL_SID_PIN_CS);
}

void RealSidChip::hwReset() {
    if (phiSet != phiHz) {
        phiSet = phiHz;
        startPhi2(phiSet);
    }
    pinHigh(REAL_SID_PIN_CS);
    pinLow(REAL_SID_PIN_RES);
    esp_rom_delay_us(20);              // co najmniej 10 cykli PHI2
    pinHigh(REAL_SID_PIN_RES);
    esp_rom_delay_us(2);
    for (uint8_t r = 0; r <= 0x18; r++) {
        regs[r] = 0;
        hwWrite(r, 0);
    }
    muted = false;
}

void RealSidChip::applyMute(bool mute) {
    muted = mute;
    hwWrite(0x18, mute ? (regs[0x18] & 0xF0) : regs[0x18]);
}

void RealSidChip::taskEntry(void* arg) { static_cast<RealSidChip*>(arg)->task(); }

void RealSidChip::task() {
    QueueHandle_t q = (QueueHandle_t)queue;
    uint32_t lastCount = clockCount;
    int64_t lastClock = esp_timer_get_time();
    int64_t lastYield = lastClock;
    bool idle = false;
    for (;;) {
        Item it;
        bool got = xQueueReceive(q, &it, 0) == pdTRUE;
        if (!got) {
            got = xQueueReceive(q, &it, pdMS_TO_TICKS(10)) == pdTRUE;
            lastYield = esp_timer_get_time();     // zadanie spalo w kolejce
        }
        int64_t now = esp_timer_get_time();

        // emulacja stoi (pauza, stop, wczytywanie pliku) -> wycisz uklad, bo gralby ostatnia nute
        uint32_t c = clockCount;
        if (c != lastCount) { lastCount = c; lastClock = now; idle = false; }
        else if (now - lastClock > IDLE_MUTE_US) idle = true;

        if (got && it.reg == RESET_ITEM) {
            if (it.gen == gen) hwReset();
            continue;
        }
        if (got && it.gen == gen) {
            idle = false;
            // czekaj na chwile zapisu: dlugie przerwy usypiaja zadanie, krotkie odlicza petla
            for (;;) {
                now = esp_timer_get_time();
                int64_t d = it.t - now;
                if (d <= 0 || it.gen != gen) break;
                if (d > 2500 || now - lastYield > MAX_SPIN_US) {
                    vTaskDelay(1);
                    lastYield = esp_timer_get_time();
                }
            }
            if (it.gen != gen) continue;
            regs[it.reg] = it.val;
            if (it.reg == 0x18 && muted) hwWrite(0x18, it.val & 0xF0);
            else hwWrite(it.reg, it.val);
        }
        bool mute = idle || wantMute;
        if (mute != muted) applyMute(mute);
        if (now - lastYield > MAX_SPIN_US) {
            vTaskDelay(1);
            lastYield = esp_timer_get_time();
        }
    }
}

#endif
