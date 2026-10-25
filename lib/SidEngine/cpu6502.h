// Emulator procesora MOS 6502/6510 (CPU Commodore 64).
// Obsluguje wszystkie rozkazy dokumentowane, tryb dziesietny (BCD)
// oraz najczesciej uzywane rozkazy nieudokumentowane (LAX, SAX, DCP, ISC...).
#pragma once
#include <stdint.h>

class Bus {
public:
    virtual ~Bus() {}
    virtual uint8_t read(uint16_t addr) = 0;
    virtual void write(uint16_t addr, uint8_t value) = 0;
};

class Cpu6502 {
public:
    enum Flag : uint8_t {
        FLAG_C = 0x01, FLAG_Z = 0x02, FLAG_I = 0x04, FLAG_D = 0x08,
        FLAG_B = 0x10, FLAG_U = 0x20, FLAG_V = 0x40, FLAG_N = 0x80
    };

    explicit Cpu6502(Bus* bus) : bus(bus) { reset(); }

    void reset();
    // Wykonuje jeden rozkaz, zwraca liczbe cykli.
    int step();

    void push(uint8_t v) { bus->write(0x0100 | sp, v); sp--; }
    uint8_t pull() { sp++; return bus->read(0x0100 | sp); }

    uint16_t pc;
    uint8_t a, x, y, sp, p;
    bool jammed;   // procesor wykonal rozkaz KIL/JAM

private:
    Bus* bus;

    inline uint8_t rd(uint16_t addr) { return bus->read(addr); }
    inline void wr(uint16_t addr, uint8_t v) { bus->write(addr, v); }
    inline uint8_t fetch() { return rd(pc++); }
    inline uint16_t fetch16() { uint16_t lo = fetch(); return lo | (uint16_t(fetch()) << 8); }
    inline void setNZ(uint8_t v) { p = (p & ~(FLAG_N | FLAG_Z)) | (v & 0x80) | (v ? 0 : FLAG_Z); }
    inline void setFlag(uint8_t f, bool on) { p = on ? (p | f) : (p & ~f); }

    // Tryby adresowania - zwracaja adres efektywny
    inline uint16_t amZp() { return fetch(); }
    inline uint16_t amZpX() { return uint8_t(fetch() + x); }
    inline uint16_t amZpY() { return uint8_t(fetch() + y); }
    inline uint16_t amAbs() { return fetch16(); }
    inline uint16_t amAbsX() { return uint16_t(fetch16() + x); }
    inline uint16_t amAbsY() { return uint16_t(fetch16() + y); }
    inline uint16_t amIndX() {
        uint8_t z = fetch() + x;
        return rd(z) | (uint16_t(rd(uint8_t(z + 1))) << 8);
    }
    inline uint16_t amIndY() {
        uint8_t z = fetch();
        uint16_t base = rd(z) | (uint16_t(rd(uint8_t(z + 1))) << 8);
        return uint16_t(base + y);
    }

    void adc(uint8_t v);
    void sbc(uint8_t v);
    void cmp(uint8_t reg, uint8_t v);
    uint8_t asl(uint8_t v);
    uint8_t lsr(uint8_t v);
    uint8_t rol(uint8_t v);
    uint8_t ror(uint8_t v);
    void branch(bool cond);
};
