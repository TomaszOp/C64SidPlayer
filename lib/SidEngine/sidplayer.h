// Odtwarzacz plikow .sid (formaty PSID i RSID).
// Laduje muzyke do 64 KB pamieci emulowanego C64, wywoluje procedure init
// i potem procedure play (lub przerwanie IRQ) z czestotliwoscia ramki
// (50 Hz PAL / 60 Hz NTSC) albo z czestotliwoscia timera CIA.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "cpu6502.h"
#include "sidchip.h"

struct SidInfo {
    bool rsid;
    uint16_t version;
    uint16_t loadAddr, initAddr, playAddr;
    uint16_t songs, startSong;
    uint32_t speed;
    char name[33], author[33], released[33];
    bool ntsc;
    Sid::Model model;
    uint16_t dataLen;
};

class SidPlayer : public Bus {
public:
    SidPlayer();
    ~SidPlayer();

    // Wczytuje plik .sid z pamieci. Zwraca false i ustawia error() przy bledzie.
    bool load(const uint8_t* data, size_t len);
    // Uruchamia utwor (numeracja od 1). 0 = utwor domyslny z pliku.
    bool startSong(int song);
    // Generuje n probek mono 16-bit.
    void render(int16_t* out, size_t n, float gain = 1.0f);

    void init(uint32_t sampleRate) { this->sampleRate = sampleRate; }
    // -1 = automatycznie z pliku, inaczej Sid::MOS6581 / Sid::MOS8580
    void setModelOverride(int m) { modelOverride = m; }

    bool loaded() const { return image != nullptr; }
    const SidInfo& info() const { return hdr; }
    const char* error() const { return err; }
    int currentSong() const { return song; }
    uint32_t playedMs() const { return uint32_t(samplesPlayed * 1000ULL / sampleRate); }
    bool usesCiaTiming() const { return useCia; }
    float frameRateHz() const { return float(clockHz / frameCycles); }

    // Interfejs magistrali dla CPU
    uint8_t read(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

private:
    bool callRoutine(uint16_t addr, uint8_t a, uint32_t maxCycles);
    bool callIrq(uint32_t maxCycles);
    bool runUntilReturn(uint32_t maxCycles);
    void playFrame();
    void updateFrameCycles();
    void installKernalStubs();
    uint8_t ioRead(uint16_t addr);
    void ioWrite(uint16_t addr, uint8_t v);

    uint8_t* mem;        // 64 KB RAM
    uint8_t* kernal;     // 8 KB atrapa ROM KERNAL ($E000-$FFFF)
    uint8_t io[0x1000];  // rejestry $D000-$DFFF
    uint8_t* image;      // kopia danych z pliku (do restartu utworu)
    SidInfo hdr;
    const char* err;

    Cpu6502 cpu;
    SidChip sid;
    int modelOverride;
    uint32_t sampleRate;
    double clockHz;
    double frameCycles;
    double cycleAcc;
    double cyclesPerSample;
    uint64_t cpuCycles;
    uint64_t samplesPlayed;
    int song;
    bool useCia;
    uint16_t ciaLatch;
};
