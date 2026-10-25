#include "sid.h"
#include <math.h>
#include <string.h>

// Liczba podprobek oscylatora na jedna probke wyjsciowa (redukuje aliasing).
#ifndef SID_OVERSAMPLE
#define SID_OVERSAMPLE 4
#endif

// Okresy licznika obwiedni w cyklach zegara (z dokumentacji / reSID).
static const uint16_t RATE_PERIOD[16] = {
    9, 32, 63, 95, 149, 220, 267, 313, 392, 977, 1954, 3126, 3907, 11720, 19532, 31251
};

static inline uint8_t expPeriod(uint8_t env) {
    if (env >= 0x5D) return 1;
    if (env >= 0x36) return 2;
    if (env >= 0x1A) return 4;
    if (env >= 0x0E) return 8;
    if (env >= 0x06) return 16;
    return 30;
}

void Sid::init(double clk, double rate, Model m) {
    clockHz = clk;
    sampleRate = rate;
    cyclesPerSample = float(clk / rate);
    model = m;
    reset();
}

void Sid::setModel(Model m) {
    model = m;
    updateFilter();
}

void Sid::reset() {
    memset(regs, 0, sizeof(regs));
    memset(voice, 0, sizeof(voice));
    for (int i = 0; i < 3; i++) {
        voice[i].lfsr = 0x7FFFF8;
        voice[i].envState = 2;
        voice[i].lastWave = 0x800;
    }
    fc = 0; res = 0; filtRoute = 0; modeVol = 0;
    lp = bp = hp = 0;
    dcPrevIn = dcPrevOut = 0;
    updateFilter();
}

void Sid::updateFilter() {
    fc = (regs[0x15] & 0x07) | (uint16_t(regs[0x16]) << 3);
    res = regs[0x17] >> 4;
    filtRoute = regs[0x17] & 0x0F;

    float cutoff;
    float t = fc / 2047.0f;
    if (model == MOS8580) {
        // 8580: niemal liniowa charakterystyka 30 Hz .. 12 kHz
        cutoff = 30.0f + t * 11970.0f;
    } else {
        // 6581: silnie nieliniowa (wykladnicza) charakterystyka
        cutoff = 200.0f * expf(t * logf(16000.0f / 200.0f));
    }
    // filtr liczony 2x na probke -> efektywna czestotliwosc probkowania 2*fs
    float maxCut = float(sampleRate) * 0.45f;
    if (cutoff > maxCut) cutoff = maxCut;
    fCoef = 2.0f * sinf(float(M_PI) * cutoff / (2.0f * float(sampleRate)));
    float q = (model == MOS8580) ? (0.707f + res * (2.6f / 15.0f))
                                 : (0.707f + res * (2.0f / 15.0f));
    qCoef = 1.0f / q;
}

void Sid::write(uint8_t reg, uint8_t value) {
    reg &= 0x1F;
    if (reg > 0x18) return;
    regs[reg] = value;
    if (reg < 21) {
        Voice& v = voice[reg / 7];
        switch (reg % 7) {
        case 0: v.freq = (v.freq & 0xFF00) | value; break;
        case 1: v.freq = (v.freq & 0x00FF) | (uint16_t(value) << 8); break;
        case 2: v.pw = (v.pw & 0x0F00) | value; break;
        case 3: v.pw = (v.pw & 0x00FF) | (uint16_t(value & 0x0F) << 8); break;
        case 4: {
            uint8_t old = v.ctrl;
            v.ctrl = value;
            if ((value & 0x01) && !(old & 0x01)) {
                v.envState = 0;              // gate 0->1: attack
            } else if (!(value & 0x01) && (old & 0x01)) {
                v.envState = 2;              // gate 1->0: release
            }
            if (value & 0x08) {              // bit TEST zeruje oscylator
                v.acc = 0;
                v.lfsr = 0x7FFFFF;
            }
            break;
        }
        case 5: v.ad = value; break;
        case 6: v.sr = value; break;
        }
    } else if (reg <= 0x17) {
        updateFilter();
    } else {
        modeVol = value;
    }
}

uint8_t Sid::read(uint8_t reg) {
    switch (reg & 0x1F) {
    case 0x19: case 0x1A: return 0xFF;                 // potencjometry (paddle)
    case 0x1B: return uint8_t(voice[2].lastWave >> 4); // OSC3
    case 0x1C: return voice[2].env;                    // ENV3
    default: return 0;
    }
}

uint16_t Sid::waveform(int i) {
    Voice& v = voice[i];
    uint8_t wave = v.ctrl >> 4;
    if (!wave) return 0x800;
    uint32_t a = v.acc >> 8;   // 24-bitowy akumulator
    uint16_t out = 0xFFF;

    if (wave & 0x1) {          // trojkat (z ewentualna modulacja pierscieniowa)
        uint32_t msb = a & 0x800000;
        if (v.ctrl & 0x04) msb ^= (voice[(i + 2) % 3].acc >> 8) & 0x800000;
        out &= uint16_t(((msb ? ~a : a) >> 11) & 0xFFF);
    }
    if (wave & 0x2) out &= uint16_t(a >> 12);               // pila
    if (wave & 0x4) {                                       // prostokat
        bool high = (v.ctrl & 0x08) || ((a >> 12) >= v.pw);
        out &= high ? 0xFFF : 0x000;
    }
    if (wave & 0x8) {                                       // szum
        uint32_t r = v.lfsr;
        uint16_t n = uint16_t((((r >> 22) & 1) << 7) | (((r >> 20) & 1) << 6) |
                              (((r >> 16) & 1) << 5) | (((r >> 13) & 1) << 4) |
                              (((r >> 11) & 1) << 3) | (((r >> 7) & 1) << 2) |
                              (((r >> 4) & 1) << 1) | ((r >> 2) & 1));
        out &= uint16_t(n << 4);
    }
    return out;
}

void Sid::clockOscillators(uint32_t inc[3]) {
    for (int i = 0; i < 3; i++) {
        Voice& v = voice[i];
        v.msbRising = false;
        if (v.ctrl & 0x08) { v.acc = 0; continue; }
        uint32_t old = v.acc;
        v.acc = old + inc[i];
        v.msbRising = !(old & 0x80000000u) && (v.acc & 0x80000000u);
        // LFSR szumu taktowany zboczem bitu 19 akumulatora (bit 27 tutaj)
        uint32_t edges = ((v.acc >> 28) - (old >> 28)) & 0x0F;
        while (edges--) {
            uint32_t bit = ((v.lfsr >> 22) ^ (v.lfsr >> 17)) & 1;
            v.lfsr = ((v.lfsr << 1) | bit) & 0x7FFFFF;
        }
    }
    // twarda synchronizacja: glos i jest resetowany przez glos (i+2)%3
    for (int i = 0; i < 3; i++) {
        if ((voice[i].ctrl & 0x02) && voice[(i + 2) % 3].msbRising) voice[i].acc = 0;
    }
}

void Sid::clockEnvelope(Voice& v, float cycles) {
    v.rateAcc += cycles;
    for (;;) {
        uint16_t period;
        if (v.envState == 0) period = RATE_PERIOD[v.ad >> 4];
        else if (v.envState == 1) period = RATE_PERIOD[v.ad & 0x0F];
        else period = RATE_PERIOD[v.sr & 0x0F];
        if (v.rateAcc < period) break;
        v.rateAcc -= period;

        if (v.envState == 0) {
            v.expCounter = 0;
            if (v.env < 0xFF) v.env++;
            if (v.env == 0xFF) v.envState = 1;
        } else {
            if (++v.expCounter < expPeriod(v.env)) continue;
            v.expCounter = 0;
            if (v.envState == 1) {
                uint8_t sustain = (v.sr >> 4) * 0x11;
                if (v.env > sustain) v.env--;
            } else if (v.env > 0) {
                v.env--;
            }
        }
    }
}

float Sid::clock() {
    const float incPerFreq = cyclesPerSample / SID_OVERSAMPLE * 256.0f;
    uint32_t inc[3];
    for (int i = 0; i < 3; i++) inc[i] = uint32_t(voice[i].freq * incPerFreq);

    int32_t waveSum[3] = {0, 0, 0};
    for (int s = 0; s < SID_OVERSAMPLE; s++) {
        clockOscillators(inc);
        for (int i = 0; i < 3; i++) {
            uint16_t w = waveform(i);
            voice[i].lastWave = w;
            waveSum[i] += int32_t(w) - 0x800;
        }
    }

    float direct = 0.0f, filtIn = 0.0f;
    const float scale = 1.0f / (2048.0f * 255.0f * SID_OVERSAMPLE);
    for (int i = 0; i < 3; i++) {
        clockEnvelope(voice[i], cyclesPerSample);
        float o = waveSum[i] * float(voice[i].env) * scale;
        if (filtRoute & (1 << i)) filtIn += o;
        else if (!(i == 2 && (modeVol & 0x80))) direct += o;   // 3OFF
    }

    float filtOut = 0.0f;
    if (modeVol & 0x70) {
        for (int k = 0; k < 2; k++) {
            hp = filtIn - lp - qCoef * bp;
            bp += fCoef * hp;
            lp += fCoef * bp;
        }
        if (modeVol & 0x10) filtOut += lp;
        if (modeVol & 0x20) filtOut += bp;
        if (modeVol & 0x40) filtOut += hp;
    } else {
        lp = bp = hp = 0.0f;
    }

    float vol = (modeVol & 0x0F) / 15.0f;
    float out = (direct + filtOut) * vol * 0.33f;
    // przesuniecie DC miksera zalezne od glosnosci (efekt "digi" w 6581)
    out += vol * (model == MOS6581 ? 0.12f : 0.01f);

    // filtr gornoprzepustowy ~16 Hz jak na wyjsciu audio C64
    float y = out - dcPrevIn + 0.9977f * dcPrevOut;
    dcPrevIn = out;
    dcPrevOut = y;
    return y;
}
