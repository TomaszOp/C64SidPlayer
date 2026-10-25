/**
 * @file real_sid_chip.h
 * @brief Real SID chip (MOS 6581 / 8580) connected to the ESP32 - same interface as Sid and ResidChip.
 *
 * Enabled with the -DSID_ENGINE=2 flag. Wiring diagram: docs/REAL_SID.md.
 *
 * The emulated 6510 computes a whole frame at once, so register writes first go into a queue
 * with a timestamp (C64 cycle) and a separate task sends them to the chip with the same spacing
 * as on a C64, delayed by REAL_SID_LATENCY_MS. The chip is write-only (R/W tied to GND); reads
 * ($D419-$D41C: paddles, OSC3, ENV3) are answered by a reSID instance running in parallel.
 *
 * Connection: two 74HCT595 shift registers (data D0-D7 in the first, address A0-A4 in the second),
 * CS, RES and the PHI2 clock (~1 MHz from LEDC) directly from the ESP32 through a 74HCT125 buffer.
 */
#pragma once
#include <stdint.h>
#include "resid_chip.h"

#if __has_include("config.h")
#include "config.h"
#endif

#ifndef REAL_SID_MODEL
/// Model of the chip in the socket: 0 = 6581 (VDD 12 V), 1 = 8580 (VDD 9 V).
#define REAL_SID_MODEL 1
#endif

// Default pins for ESP32-S3 / classic ESP32 (they do not collide with I2S and the SD card).
#if CONFIG_IDF_TARGET_ESP32S3
#define REAL_SID_DEF_DATA  15
#define REAL_SID_DEF_SHIFT 16
#define REAL_SID_DEF_LATCH 17
#define REAL_SID_DEF_CS    18
#define REAL_SID_DEF_PHI2  7
#define REAL_SID_DEF_RES   8
#else
#define REAL_SID_DEF_DATA  13
#define REAL_SID_DEF_SHIFT 14
#define REAL_SID_DEF_LATCH 27
#define REAL_SID_DEF_CS    21
#define REAL_SID_DEF_PHI2  32
#define REAL_SID_DEF_RES   33
#endif
#ifndef REAL_SID_PIN_DATA
#define REAL_SID_PIN_DATA  REAL_SID_DEF_DATA     ///< 74HCT595 DS (pin 14) of the first register.
#endif
#ifndef REAL_SID_PIN_SHIFT
#define REAL_SID_PIN_SHIFT REAL_SID_DEF_SHIFT    ///< SHCP (pin 11) of both registers.
#endif
#ifndef REAL_SID_PIN_LATCH
#define REAL_SID_PIN_LATCH REAL_SID_DEF_LATCH    ///< STCP (pin 12) of both registers.
#endif
#ifndef REAL_SID_PIN_CS
#define REAL_SID_PIN_CS    REAL_SID_DEF_CS       ///< SID /CS (pin 8).
#endif
#ifndef REAL_SID_PIN_PHI2
#define REAL_SID_PIN_PHI2  REAL_SID_DEF_PHI2     ///< SID PHI2 (pin 6).
#endif
#ifndef REAL_SID_PIN_RES
#define REAL_SID_PIN_RES   REAL_SID_DEF_RES      ///< SID /RES (pin 5).
#endif

#ifndef REAL_SID_LATENCY_MS
/// Delay of the chip writes relative to the emulation in ms (must cover uneven frame computation).
#define REAL_SID_LATENCY_MS 30
#endif
#ifndef REAL_SID_EMU_AUDIO
/// 1 = reSID also plays through I2S (to compare the emulation with the chip), 0 = silence on I2S.
#define REAL_SID_EMU_AUDIO 0
#endif
#ifndef REAL_SID_TASK_CORE
/// CPU core of the task that sends writes to the chip (the audio task runs on core 1).
#define REAL_SID_TASK_CORE 0
#endif
#ifndef REAL_SID_TASK_PRIORITY
/// FreeRTOS priority of the chip write task.
#define REAL_SID_TASK_PRIORITY 3
#endif
#ifndef REAL_SID_QUEUE_LEN
/// Capacity of the timed write queue (entries).
#define REAL_SID_QUEUE_LEN 1024
#endif

/**
 * @brief SID engine that drives a real 6581/8580 chip through shift registers, with a reSID shadow for reads.
 */
class RealSidChip {
public:
    /**
     * @brief Starts the hardware on first use, initializes the reSID shadow and resets the chip.
     *
     * The model from the file is ignored - the chip in the socket (REAL_SID_MODEL) is what counts.
     * @param clockHz    C64 clock in Hz; also used as the PHI2 frequency.
     * @param sampleRate Output sample rate in Hz (for the reSID shadow).
     * @param m          Ignored.
     */
    void init(double clockHz, double sampleRate, Sid::Model m);
    /** @brief Resets the shadow, drops queued writes of the previous tune and queues a chip reset. */
    void reset();
    /** @brief Does nothing - the model is fixed by the chip in the socket. */
    void setModel(Sid::Model) {}

    /**
     * @brief Writes a register: updates the shadow and queues a timed write to the chip.
     * @param reg   Register number (only the low 5 bits are used; > $18 is not sent).
     * @param value Value to write.
     */
    void write(uint8_t reg, uint8_t value);
    /**
     * @brief Reads a register from the reSID shadow (the real chip is write-only).
     * @param reg Register number.
     * @return Register value.
     */
    uint8_t read(uint8_t reg) { return shadow.read(reg); }
    /**
     * @brief Produces one I2S sample.
     * @return reSID shadow output when REAL_SID_EMU_AUDIO is 1, otherwise 0 (silence).
     */
    float clock();

    /**
     * @brief Sets the timestamp of the next write (called by SidPlayer).
     * @param cycles Time of the write in C64 cycles since the start of the subtune.
     */
    void setTime(double cycles) { timeCycles = cycles; }
    /**
     * @brief Mutes the player (volume 0) - the chip gets the volume in $D418 zeroed.
     * @param on true = mute.
     */
    void setMuted(bool on) { wantMute = on; }

private:
    /** @brief Queued register write. */
    struct Item { int64_t t; uint16_t gen; uint8_t reg, val; };
    /**
     * @brief FreeRTOS task entry point.
     * @param arg Pointer to the RealSidChip instance.
     */
    static void taskEntry(void* arg);
    /** @brief Output task loop: waits for each write's time, sends it to the chip, mutes when idle. */
    void task();
    /** @brief Configures the pins, creates the queue and starts the output task. */
    void begin();
    /**
     * @brief Shifts a register address and value into the 74HCT595s and pulses /CS.
     * @param reg Register number.
     * @param val Value.
     */
    void hwWrite(uint8_t reg, uint8_t val);
    /** @brief (Re)starts PHI2 if needed, pulses /RES and clears all chip registers. */
    void hwReset();
    /**
     * @brief Mutes or unmutes the chip by rewriting the volume bits of $D418.
     * @param mute true = volume 0.
     */
    void applyMute(bool mute);

    ResidChip shadow;              ///< reSID instance answering register reads (and optional I2S audio).
    void* queue = nullptr;         ///< FreeRTOS queue of Item (QueueHandle_t).
    double timeCycles = 0;         ///< Timestamp of the next write in C64 cycles.
    double usPerCycle = 1.0;       ///< Microseconds per C64 cycle.
    int64_t anchorUs = 0;          ///< esp_timer time corresponding to cycle 0.
    bool anchored = false;         ///< anchorUs is valid.
    volatile uint16_t gen = 0;     ///< Generation counter; writes of older generations are dropped.
    volatile uint32_t clockCount = 0;   ///< Number of clock() calls (detects a stopped emulation).
    volatile bool wantMute = false;     ///< Mute requested by the player.
    volatile double phiHz = 985248.0;   ///< Requested PHI2 frequency in Hz.
    // state owned by the output task
    uint8_t regs[32] = {};         ///< Output task: last values written to the chip.
    bool muted = false;            ///< Output task: chip volume currently zeroed.
    double phiSet = 0;             ///< Output task: PHI2 frequency currently generated.
};
