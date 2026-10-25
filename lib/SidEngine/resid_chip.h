/**
 * @file resid_chip.h
 * @brief Wrapper around reSID with the same interface as the Sid class (sid.h).
 */
#pragma once
#include "resid/sid.h"
#include "sid.h"

/**
 * @brief SID chip emulated by reSID, exposing the same interface as Sid.
 */
class ResidChip {
public:
    /**
     * @brief Configures sampling, sets the chip model and resets the chip.
     * @param clockHz    C64 system clock in Hz (PAL or NTSC).
     * @param sampleRate Output sample rate in Hz.
     * @param m          Chip model to emulate.
     */
    void init(double clockHz, double sampleRate, Sid::Model m) {
        chip.set_sampling_parameters(float(clockHz),
            SID_RESID_INTERPOLATE ? reSID::SAMPLE_INTERPOLATE : reSID::SAMPLE_FAST,
            float(sampleRate));
        setModel(m);
        reset();
    }
    /** @brief Resets all chip registers and internal state. */
    void reset() { chip.reset(); }
    /**
     * @brief Selects the emulated chip model.
     * @param m Sid::MOS6581 or Sid::MOS8580.
     */
    void setModel(Sid::Model m) {
        chip.set_chip_model(m == Sid::MOS8580 ? reSID::MOS8580 : reSID::MOS6581);
    }
    /**
     * @brief Writes a SID register.
     * @param reg   Register number (only the low 5 bits are used).
     * @param value Value to write.
     */
    void write(uint8_t reg, uint8_t value) { chip.write(reg & 0x1F, value); }
    /**
     * @brief Reads a SID register (e.g. OSC3, ENV3, paddles).
     * @param reg Register number (only the low 5 bits are used).
     * @return Register value.
     */
    uint8_t read(uint8_t reg) { return uint8_t(chip.read(reg & 0x1F)); }

    /**
     * @brief Generates one output sample.
     *
     * reSID clocks the chip exactly up to the sample point, and the unused
     * cycles from "dt" are not discarded, so no time is lost.
     * @return Sample scaled to roughly the same loudness as the own engine (about -1.5 .. 1.5).
     */
    float clock() {
        reSID::cycle_count dt = 64;
        short s = 0;
        chip.clock(dt, &s, 1);
        return s * (1.5f / 32768.0f);   // ~ta sama glosnosc co silnik wlasny
    }

private:
    reSID::SID chip;   ///< The reSID emulator instance.
};
