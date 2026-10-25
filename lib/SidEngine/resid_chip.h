// Nakladka na reSID z tym samym interfejsem co klasa Sid (sid.h).
#pragma once
#include "resid/sid.h"
#include "sid.h"

class ResidChip {
public:
    void init(double clockHz, double sampleRate, Sid::Model m) {
        chip.set_sampling_parameters(float(clockHz),
            SID_RESID_INTERPOLATE ? reSID::SAMPLE_INTERPOLATE : reSID::SAMPLE_FAST,
            float(sampleRate));
        setModel(m);
        reset();
    }
    void reset() { chip.reset(); }
    void setModel(Sid::Model m) {
        chip.set_chip_model(m == Sid::MOS8580 ? reSID::MOS8580 : reSID::MOS6581);
    }
    void write(uint8_t reg, uint8_t value) { chip.write(reg & 0x1F, value); }
    uint8_t read(uint8_t reg) { return uint8_t(chip.read(reg & 0x1F)); }

    // Jedna probka. reSID taktuje uklad dokladnie do chwili probki,
    // a niewykorzystane cykle z "dt" nie sa liczone, wiec czas sie nie gubi.
    float clock() {
        reSID::cycle_count dt = 64;
        short s = 0;
        chip.clock(dt, &s, 1);
        return s * (1.5f / 32768.0f);   // ~ta sama glosnosc co silnik wlasny
    }

private:
    reSID::SID chip;
};
