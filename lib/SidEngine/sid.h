// Emulacja ukladu dzwiekowego MOS 6581/8580 SID.
// 3 glosy (pila, trojkat, prostokat, szum i ich kombinacje), synchronizacja,
// modulacja pierscieniowa, obwiednia ADSR z prawdziwymi czasami SID-a
// oraz filtr wielotrybowy (LP/BP/HP) z rezonansem.
#pragma once
#include <stdint.h>

class Sid {
public:
    enum Model { MOS6581, MOS8580 };

    void init(double clockHz, double sampleRate, Model model);
    void reset();
    void setModel(Model m);

    void write(uint8_t reg, uint8_t value);
    uint8_t read(uint8_t reg);

    // Generuje kolejna probke audio (zakres okolo -1.0 .. 1.0).
    float clock();

private:
    struct Voice {
        uint32_t acc;        // 24-bitowy akumulator fazy (bity 8..31) + 8 bitow ulamka
        uint32_t lfsr;       // 23-bitowy rejestr szumu
        uint16_t freq, pw;
        uint8_t ctrl, ad, sr;
        uint8_t envState;    // 0 = attack, 1 = decay/sustain, 2 = release
        uint8_t env;         // poziom obwiedni 0..255
        float rateAcc;       // licznik cykli do nastepnego kroku obwiedni
        uint8_t expCounter;
        bool msbRising;      // MSB akumulatora wlasnie przeszedl 0->1 (dla sync)
        uint16_t lastWave;   // ostatnia wartosc 12-bitowa (dla odczytu OSC3)
    };

    uint16_t waveform(int v);
    void clockOscillators(uint32_t inc[3]);
    void clockEnvelope(Voice& v, float cycles);
    void updateFilter();

    Voice voice[3];
    uint8_t regs[32];
    Model model;
    double clockHz, sampleRate;
    float cyclesPerSample;
    uint32_t incScale;       // freq * incScale / OVERSAMPLE = przyrost akumulatora na podprobke

    // Filtr (Chamberlin state variable, liczony 2x na probke)
    uint16_t fc;
    uint8_t res, filtRoute, modeVol;
    float fCoef, qCoef;
    float lp, bp, hp;
    // Filtr wyjsciowy (DC block) jak na plycie C64
    float dcPrevIn, dcPrevOut;
};
