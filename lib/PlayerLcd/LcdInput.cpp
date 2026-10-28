/**
 * @file LcdInput.cpp
 * @brief Implementation of LcdInput: PCNT quadrature decoding and button debouncing.
 */
#include "LcdInput.h"
#if USE_LCD
#include <driver/pcnt.h>

static const pcnt_unit_t ENC_UNIT = PCNT_UNIT_0;   ///< PCNT unit used by the encoder.
static const uint32_t DEBOUNCE_MS = 25;            ///< Button debounce time.
static const uint32_t FAST_TURN_MS = 60;           ///< Turns closer than this are "fast".

void LcdInput::begin(const LcdInputConfig& cfg) {
    cfg_ = cfg;
    push_.pin = cfg.pinPush;
    back_.pin = cfg.pinBack;
    select_.pin = cfg.pinSelect;
    for (Button* b : {&push_, &back_, &select_})
        if (b->pin >= 0) pinMode(b->pin, INPUT_PULLUP);
    lastActivity_ = millis();
    if (cfg.pinA < 0 || cfg.pinB < 0) return;

    // dekodowanie kwadraturowe x4: oba kanaly licza oba zbocza, drugi pin wyznacza kierunek
    pinMode(cfg.pinA, INPUT_PULLUP);
    pinMode(cfg.pinB, INPUT_PULLUP);
    pcnt_config_t c = {};
    c.pulse_gpio_num = cfg.pinA;
    c.ctrl_gpio_num = cfg.pinB;
    c.channel = PCNT_CHANNEL_0;
    c.unit = ENC_UNIT;
    c.pos_mode = PCNT_COUNT_DEC;
    c.neg_mode = PCNT_COUNT_INC;
    c.lctrl_mode = PCNT_MODE_REVERSE;
    c.hctrl_mode = PCNT_MODE_KEEP;
    c.counter_h_lim = 32767;
    c.counter_l_lim = -32768;
    pcnt_unit_config(&c);
    c.pulse_gpio_num = cfg.pinB;
    c.ctrl_gpio_num = cfg.pinA;
    c.channel = PCNT_CHANNEL_1;
    c.pos_mode = PCNT_COUNT_INC;
    c.neg_mode = PCNT_COUNT_DEC;
    pcnt_unit_config(&c);
    pcnt_set_filter_value(ENC_UNIT, 1023);   // ~12.8 us at 80 MHz APB - filters contact bounce
    pcnt_filter_enable(ENC_UNIT);
    pcnt_counter_pause(ENC_UNIT);
    pcnt_counter_clear(ENC_UNIT);
    pcnt_counter_resume(ENC_UNIT);
    ready_ = true;
}

int LcdInput::readDetents() {
    if (!ready_) return 0;
    int16_t raw = 0;
    pcnt_get_counter_value(ENC_UNIT, &raw);
    acc_ += int(raw) - int(lastRaw_);
    lastRaw_ = raw;
    if (raw > 16000 || raw < -16000) {   // daleko od przepelnienia licznika - wyzeruj
        pcnt_counter_clear(ENC_UNIT);
        lastRaw_ = 0;
    }
    int per = cfg_.countsPerDetent ? cfg_.countsPerDetent : 4;
    int steps = acc_ / per;
    acc_ -= steps * per;
    return cfg_.reverse ? -steps : steps;
}

int8_t LcdInput::update(Button& b) {
    if (b.pin < 0) return 0;
    bool raw = digitalRead(b.pin) == LOW;
    uint32_t now = millis();
    if (raw != b.raw) { b.raw = raw; b.rawSince = now; }
    if (raw == b.down || now - b.rawSince < DEBOUNCE_MS) return 0;
    b.down = raw;
    lastActivity_ = now;
    if (raw) { b.downSince = now; b.level = 0; return 1; }
    return -1;
}

LcdEvent LcdInput::poll() {
    uint32_t now = millis();
    int8_t p = update(push_);
    uint32_t held = now - push_.downSince;
    if (encoderOnly()) {
        // krotko = Wybierz, dlugo = Wstecz (po puszczeniu), bardzo dlugo = Start
        if (p < 0) {
            uint8_t level = push_.level;
            push_.level = 0;
            if (level == 0) return LcdEvent::Select;
            if (level == 1) return LcdEvent::Back;
        }
        if (push_.down && push_.level < 2 && held >= cfg_.veryLongMs) { push_.level = 2; return LcdEvent::Home; }
        if (push_.down && push_.level < 1 && held >= cfg_.longMs) { push_.level = 1; return LcdEvent::HoldHint; }
    } else {
        if (p < 0 && push_.level == 0) return LcdEvent::PlayPause;
        if (push_.down && push_.level == 0 && held >= cfg_.longMs) { push_.level = 1; return LcdEvent::Stop; }

        int8_t b = update(back_);
        if (b < 0 && back_.level == 0) return LcdEvent::Back;
        if (back_.down && back_.level == 0 && now - back_.downSince >= cfg_.longMs) {
            back_.level = 1;
            return LcdEvent::Home;
        }
        if (update(select_) < 0) return LcdEvent::Select;
    }

    int steps = readDetents();
    if (steps) {
        fast_ = now - lastTurn_ < FAST_TURN_MS || steps > 1 || steps < -1;
        lastTurn_ = now;
        lastActivity_ = now;
        turn_ = steps;
        return LcdEvent::Turn;
    }
    return LcdEvent::None;
}
#endif
