/**
 * @file sid.h
 * @brief Emulation of the MOS 6581/8580 SID sound chip.
 *
 * 3 voices (sawtooth, triangle, pulse, noise and their combinations), hard sync,
 * ring modulation, ADSR envelope with real SID timings
 * and a multimode filter (LP/BP/HP) with resonance.
 */
#pragma once
#include <stdint.h>

/**
 * @brief Own lightweight SID chip emulation.
 */
class Sid {
public:
    /** @brief SID chip model. */
    enum Model { MOS6581, MOS8580 };

    /**
     * @brief Initializes the emulation and resets the chip.
     * @param clockHz    C64 system clock in Hz (PAL or NTSC).
     * @param sampleRate Output sample rate in Hz.
     * @param model      Chip model to emulate.
     */
    void init(double clockHz, double sampleRate, Model model);
    /** @brief Clears all registers, voices and filter state (all voices start in release). */
    void reset();
    /**
     * @brief Changes the chip model and recalculates the filter coefficients.
     * @param m New chip model.
     */
    void setModel(Model m);

    /**
     * @brief Writes a SID register ($D400 + reg); registers above $18 are ignored.
     * @param reg   Register number (only the low 5 bits are used).
     * @param value Value to write.
     */
    void write(uint8_t reg, uint8_t value);
    /**
     * @brief Reads a SID register.
     * @param reg Register number (only the low 5 bits are used).
     * @return $FF for the paddles ($19/$1A), OSC3 ($1B), ENV3 ($1C), otherwise 0.
     */
    uint8_t read(uint8_t reg);

    /**
     * @brief Generates the next audio sample.
     * @return Sample in the range of about -1.0 .. 1.0.
     */
    float clock();

private:
    /** @brief State of one SID voice. */
    struct Voice {
        uint32_t acc;        ///< 24-bit phase accumulator (bits 8..31) + 8 fraction bits.
        uint32_t lfsr;       ///< 23-bit noise shift register.
        uint16_t freq, pw;   ///< Frequency register and 12-bit pulse width.
        uint8_t ctrl, ad, sr;   ///< Control, attack/decay and sustain/release registers.
        uint8_t envState;    ///< 0 = attack, 1 = decay/sustain, 2 = release.
        uint8_t env;         ///< Envelope level 0..255.
        float rateAcc;       ///< Cycle counter until the next envelope step.
        uint8_t expCounter;  ///< Exponential decay/release counter.
        bool msbRising;      ///< Accumulator MSB just went 0->1 (for sync).
        uint16_t lastWave;   ///< Last 12-bit waveform value (for reading OSC3).
    };

    /**
     * @brief Computes the current waveform output of a voice.
     * @param v Voice index 0..2.
     * @return 12-bit waveform value ($800 when no waveform is selected).
     */
    uint16_t waveform(int v);
    /**
     * @brief Advances all three oscillators by one subsample, clocks noise and applies hard sync.
     * @param inc Accumulator increment for each voice.
     */
    void clockOscillators(uint32_t inc[3]);
    /**
     * @brief Advances the ADSR envelope of a voice.
     * @param v      Voice to update.
     * @param cycles Number of C64 clock cycles that elapsed.
     */
    void clockEnvelope(Voice& v, float cycles);
    /** @brief Recalculates the filter cutoff/resonance coefficients from registers $15-$17. */
    void updateFilter();

    Voice voice[3];          ///< The three voices.
    uint8_t regs[32];        ///< Shadow copy of the written registers.
    Model model;             ///< Emulated chip model.
    double clockHz, sampleRate;   ///< C64 clock and output sample rate in Hz.
    float cyclesPerSample;   ///< C64 cycles per output sample.
    uint32_t incScale;       ///< freq * incScale / OVERSAMPLE = accumulator increment per subsample.

    // Filter (Chamberlin state variable, computed 2x per sample)
    uint16_t fc;             ///< 11-bit filter cutoff register value.
    uint8_t res, filtRoute, modeVol;   ///< Resonance, voice routing to the filter, mode/volume register ($18).
    float fCoef, qCoef;      ///< Chamberlin state-variable filter coefficients (frequency and 1/Q), run 2x per sample.
    float lp, bp, hp;        ///< Low-pass, band-pass and high-pass filter state.
    // Output filter (DC block) like on the C64 board
    float dcPrevIn, dcPrevOut;   ///< Output DC-blocking filter state (like on the C64 board).
};
