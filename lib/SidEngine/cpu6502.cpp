/**
 * @file cpu6502.cpp
 * @brief Implementation of the MOS 6502/6510 CPU emulator.
 */
#include "cpu6502.h"

/// Base cycle count for every opcode (without page-crossing penalties); 0 = KIL/JAM.
static const uint8_t CYCLES[256] = {
    7,6,0,8,3,3,5,5,3,2,2,2,4,4,6,6, 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    6,6,0,8,3,3,5,5,4,2,2,2,4,4,6,6, 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    6,6,0,8,3,3,5,5,3,2,2,2,3,4,6,6, 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    6,6,0,8,3,3,5,5,4,2,2,2,5,4,6,6, 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4, 2,6,0,6,4,4,4,4,2,5,2,5,5,5,5,5,
    2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4, 2,5,0,5,4,4,4,4,2,4,2,4,4,4,4,4,
    2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6, 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6, 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
};

void Cpu6502::reset() {
    pc = 0; a = x = y = 0; sp = 0xFD; p = FLAG_U | FLAG_I; jammed = false;
}

void Cpu6502::adc(uint8_t v) {
    unsigned c = p & FLAG_C;
    if (p & FLAG_D) {
        unsigned tmp = (a & 0x0F) + (v & 0x0F) + c;
        if (tmp > 9) tmp += 6;
        if (tmp <= 0x0F) tmp = (tmp & 0x0F) + (a & 0xF0) + (v & 0xF0);
        else tmp = (tmp & 0x0F) + (a & 0xF0) + (v & 0xF0) + 0x10;
        setFlag(FLAG_Z, ((a + v + c) & 0xFF) == 0);
        setFlag(FLAG_N, tmp & 0x80);
        setFlag(FLAG_V, ((a ^ tmp) & 0x80) && !((a ^ v) & 0x80));
        if ((tmp & 0x1F0) > 0x90) tmp += 0x60;
        setFlag(FLAG_C, (tmp & 0xFF0) > 0xF0);
        a = uint8_t(tmp);
    } else {
        unsigned sum = a + v + c;
        setFlag(FLAG_V, !((a ^ v) & 0x80) && ((a ^ sum) & 0x80));
        setFlag(FLAG_C, sum > 0xFF);
        a = uint8_t(sum);
        setNZ(a);
    }
}

void Cpu6502::sbc(uint8_t v) {
    unsigned borrow = (p & FLAG_C) ? 0 : 1;
    unsigned tmp = a - v - borrow;
    if (p & FLAG_D) {
        unsigned tmpA = (a & 0x0F) - (v & 0x0F) - borrow;
        if (tmpA & 0x10) tmpA = ((tmpA - 6) & 0x0F) | ((a & 0xF0) - (v & 0xF0) - 0x10);
        else tmpA = (tmpA & 0x0F) | ((a & 0xF0) - (v & 0xF0));
        if (tmpA & 0x100) tmpA -= 0x60;
        setFlag(FLAG_C, tmp < 0x100);
        setNZ(uint8_t(tmp));
        setFlag(FLAG_V, ((a ^ tmp) & 0x80) && ((a ^ v) & 0x80));
        a = uint8_t(tmpA);
    } else {
        setFlag(FLAG_C, tmp < 0x100);
        setFlag(FLAG_V, ((a ^ tmp) & 0x80) && ((a ^ v) & 0x80));
        a = uint8_t(tmp);
        setNZ(a);
    }
}

void Cpu6502::cmp(uint8_t reg, uint8_t v) {
    unsigned t = reg - v;
    setFlag(FLAG_C, reg >= v);
    setNZ(uint8_t(t));
}

uint8_t Cpu6502::asl(uint8_t v) { setFlag(FLAG_C, v & 0x80); v <<= 1; setNZ(v); return v; }
uint8_t Cpu6502::lsr(uint8_t v) { setFlag(FLAG_C, v & 0x01); v >>= 1; setNZ(v); return v; }
uint8_t Cpu6502::rol(uint8_t v) {
    uint8_t c = p & FLAG_C;
    setFlag(FLAG_C, v & 0x80); v = uint8_t((v << 1) | c); setNZ(v); return v;
}
uint8_t Cpu6502::ror(uint8_t v) {
    uint8_t c = (p & FLAG_C) ? 0x80 : 0;
    setFlag(FLAG_C, v & 0x01); v = uint8_t((v >> 1) | c); setNZ(v); return v;
}

void Cpu6502::branch(bool cond) {
    int8_t off = int8_t(fetch());
    if (cond) pc = uint16_t(pc + off);
}

int Cpu6502::step() {
    if (jammed) return 2;
    uint8_t op = fetch();
    uint16_t ad;
    uint8_t v;

// Makra skracajace powtarzalne grupy rozkazow
#define LOAD(reg, mode) { reg = rd(mode); setNZ(reg); break; }
#define STORE(reg, mode) { wr(mode, reg); break; }
#define RMW(fn, mode) { ad = mode; v = rd(ad); wr(ad, v); v = fn(v); wr(ad, v); break; }

    switch (op) {
    // --- LDA/LDX/LDY ---
    case 0xA9: a = fetch(); setNZ(a); break;
    case 0xA5: LOAD(a, amZp());
    case 0xB5: LOAD(a, amZpX());
    case 0xAD: LOAD(a, amAbs());
    case 0xBD: LOAD(a, amAbsX());
    case 0xB9: LOAD(a, amAbsY());
    case 0xA1: LOAD(a, amIndX());
    case 0xB1: LOAD(a, amIndY());
    case 0xA2: x = fetch(); setNZ(x); break;
    case 0xA6: LOAD(x, amZp());
    case 0xB6: LOAD(x, amZpY());
    case 0xAE: LOAD(x, amAbs());
    case 0xBE: LOAD(x, amAbsY());
    case 0xA0: y = fetch(); setNZ(y); break;
    case 0xA4: LOAD(y, amZp());
    case 0xB4: LOAD(y, amZpX());
    case 0xAC: LOAD(y, amAbs());
    case 0xBC: LOAD(y, amAbsX());
    // --- STA/STX/STY ---
    case 0x85: STORE(a, amZp());
    case 0x95: STORE(a, amZpX());
    case 0x8D: STORE(a, amAbs());
    case 0x9D: STORE(a, amAbsX());
    case 0x99: STORE(a, amAbsY());
    case 0x81: STORE(a, amIndX());
    case 0x91: STORE(a, amIndY());
    case 0x86: STORE(x, amZp());
    case 0x96: STORE(x, amZpY());
    case 0x8E: STORE(x, amAbs());
    case 0x84: STORE(y, amZp());
    case 0x94: STORE(y, amZpX());
    case 0x8C: STORE(y, amAbs());
    // --- transfery ---
    case 0xAA: x = a; setNZ(x); break;
    case 0xA8: y = a; setNZ(y); break;
    case 0x8A: a = x; setNZ(a); break;
    case 0x98: a = y; setNZ(a); break;
    case 0xBA: x = sp; setNZ(x); break;
    case 0x9A: sp = x; break;
    // --- stos ---
    case 0x48: push(a); break;
    case 0x08: push(p | FLAG_B | FLAG_U); break;
    case 0x68: a = pull(); setNZ(a); break;
    case 0x28: p = (pull() & ~FLAG_B) | FLAG_U; break;
    // --- logika ---
    case 0x29: a &= fetch(); setNZ(a); break;
    case 0x25: a &= rd(amZp()); setNZ(a); break;
    case 0x35: a &= rd(amZpX()); setNZ(a); break;
    case 0x2D: a &= rd(amAbs()); setNZ(a); break;
    case 0x3D: a &= rd(amAbsX()); setNZ(a); break;
    case 0x39: a &= rd(amAbsY()); setNZ(a); break;
    case 0x21: a &= rd(amIndX()); setNZ(a); break;
    case 0x31: a &= rd(amIndY()); setNZ(a); break;
    case 0x09: a |= fetch(); setNZ(a); break;
    case 0x05: a |= rd(amZp()); setNZ(a); break;
    case 0x15: a |= rd(amZpX()); setNZ(a); break;
    case 0x0D: a |= rd(amAbs()); setNZ(a); break;
    case 0x1D: a |= rd(amAbsX()); setNZ(a); break;
    case 0x19: a |= rd(amAbsY()); setNZ(a); break;
    case 0x01: a |= rd(amIndX()); setNZ(a); break;
    case 0x11: a |= rd(amIndY()); setNZ(a); break;
    case 0x49: a ^= fetch(); setNZ(a); break;
    case 0x45: a ^= rd(amZp()); setNZ(a); break;
    case 0x55: a ^= rd(amZpX()); setNZ(a); break;
    case 0x4D: a ^= rd(amAbs()); setNZ(a); break;
    case 0x5D: a ^= rd(amAbsX()); setNZ(a); break;
    case 0x59: a ^= rd(amAbsY()); setNZ(a); break;
    case 0x41: a ^= rd(amIndX()); setNZ(a); break;
    case 0x51: a ^= rd(amIndY()); setNZ(a); break;
    case 0x24: case 0x2C:
        v = rd(op == 0x24 ? amZp() : amAbs());
        p = (p & ~(FLAG_N | FLAG_V | FLAG_Z)) | (v & 0xC0) | ((a & v) ? 0 : FLAG_Z);
        break;
    // --- arytmetyka ---
    case 0x69: adc(fetch()); break;
    case 0x65: adc(rd(amZp())); break;
    case 0x75: adc(rd(amZpX())); break;
    case 0x6D: adc(rd(amAbs())); break;
    case 0x7D: adc(rd(amAbsX())); break;
    case 0x79: adc(rd(amAbsY())); break;
    case 0x61: adc(rd(amIndX())); break;
    case 0x71: adc(rd(amIndY())); break;
    case 0xE9: case 0xEB: sbc(fetch()); break;
    case 0xE5: sbc(rd(amZp())); break;
    case 0xF5: sbc(rd(amZpX())); break;
    case 0xED: sbc(rd(amAbs())); break;
    case 0xFD: sbc(rd(amAbsX())); break;
    case 0xF9: sbc(rd(amAbsY())); break;
    case 0xE1: sbc(rd(amIndX())); break;
    case 0xF1: sbc(rd(amIndY())); break;
    case 0xC9: cmp(a, fetch()); break;
    case 0xC5: cmp(a, rd(amZp())); break;
    case 0xD5: cmp(a, rd(amZpX())); break;
    case 0xCD: cmp(a, rd(amAbs())); break;
    case 0xDD: cmp(a, rd(amAbsX())); break;
    case 0xD9: cmp(a, rd(amAbsY())); break;
    case 0xC1: cmp(a, rd(amIndX())); break;
    case 0xD1: cmp(a, rd(amIndY())); break;
    case 0xE0: cmp(x, fetch()); break;
    case 0xE4: cmp(x, rd(amZp())); break;
    case 0xEC: cmp(x, rd(amAbs())); break;
    case 0xC0: cmp(y, fetch()); break;
    case 0xC4: cmp(y, rd(amZp())); break;
    case 0xCC: cmp(y, rd(amAbs())); break;
    // --- INC/DEC ---
    case 0xE6: ad = amZp(); v = rd(ad) + 1; wr(ad, v); setNZ(v); break;
    case 0xF6: ad = amZpX(); v = rd(ad) + 1; wr(ad, v); setNZ(v); break;
    case 0xEE: ad = amAbs(); v = rd(ad) + 1; wr(ad, v); setNZ(v); break;
    case 0xFE: ad = amAbsX(); v = rd(ad) + 1; wr(ad, v); setNZ(v); break;
    case 0xC6: ad = amZp(); v = rd(ad) - 1; wr(ad, v); setNZ(v); break;
    case 0xD6: ad = amZpX(); v = rd(ad) - 1; wr(ad, v); setNZ(v); break;
    case 0xCE: ad = amAbs(); v = rd(ad) - 1; wr(ad, v); setNZ(v); break;
    case 0xDE: ad = amAbsX(); v = rd(ad) - 1; wr(ad, v); setNZ(v); break;
    case 0xE8: x++; setNZ(x); break;
    case 0xC8: y++; setNZ(y); break;
    case 0xCA: x--; setNZ(x); break;
    case 0x88: y--; setNZ(y); break;
    // --- przesuniecia ---
    case 0x0A: a = asl(a); break;
    case 0x06: RMW(asl, amZp());
    case 0x16: RMW(asl, amZpX());
    case 0x0E: RMW(asl, amAbs());
    case 0x1E: RMW(asl, amAbsX());
    case 0x4A: a = lsr(a); break;
    case 0x46: RMW(lsr, amZp());
    case 0x56: RMW(lsr, amZpX());
    case 0x4E: RMW(lsr, amAbs());
    case 0x5E: RMW(lsr, amAbsX());
    case 0x2A: a = rol(a); break;
    case 0x26: RMW(rol, amZp());
    case 0x36: RMW(rol, amZpX());
    case 0x2E: RMW(rol, amAbs());
    case 0x3E: RMW(rol, amAbsX());
    case 0x6A: a = ror(a); break;
    case 0x66: RMW(ror, amZp());
    case 0x76: RMW(ror, amZpX());
    case 0x6E: RMW(ror, amAbs());
    case 0x7E: RMW(ror, amAbsX());
    // --- skoki ---
    case 0x4C: pc = fetch16(); break;
    case 0x6C: {
        uint16_t ptr = fetch16();
        // blad sprzetowy 6502: JMP ($xxFF) czyta starszy bajt z $xx00
        uint16_t hiAddr = (ptr & 0xFF00) | uint8_t(ptr + 1);
        pc = rd(ptr) | (uint16_t(rd(hiAddr)) << 8);
        break;
    }
    case 0x20: {
        uint16_t target = fetch16();
        uint16_t ret = pc - 1;
        push(ret >> 8); push(ret & 0xFF);
        pc = target;
        break;
    }
    case 0x60: { uint16_t lo = pull(); pc = uint16_t((lo | (uint16_t(pull()) << 8)) + 1); break; }
    case 0x40: {
        p = (pull() & ~FLAG_B) | FLAG_U;
        uint16_t lo = pull(); pc = lo | (uint16_t(pull()) << 8);
        break;
    }
    case 0x00: {
        pc++;
        push(pc >> 8); push(pc & 0xFF); push(p | FLAG_B | FLAG_U);
        p |= FLAG_I;
        pc = rd(0xFFFE) | (uint16_t(rd(0xFFFF)) << 8);
        break;
    }
    // --- skoki warunkowe ---
    case 0x10: branch(!(p & FLAG_N)); break;
    case 0x30: branch(p & FLAG_N); break;
    case 0x50: branch(!(p & FLAG_V)); break;
    case 0x70: branch(p & FLAG_V); break;
    case 0x90: branch(!(p & FLAG_C)); break;
    case 0xB0: branch(p & FLAG_C); break;
    case 0xD0: branch(!(p & FLAG_Z)); break;
    case 0xF0: branch(p & FLAG_Z); break;
    // --- flagi ---
    case 0x18: p &= ~FLAG_C; break;
    case 0x38: p |= FLAG_C; break;
    case 0x58: p &= ~FLAG_I; break;
    case 0x78: p |= FLAG_I; break;
    case 0xB8: p &= ~FLAG_V; break;
    case 0xD8: p &= ~FLAG_D; break;
    case 0xF8: p |= FLAG_D; break;
    // --- NOP (oficjalny i nieudokumentowane) ---
    case 0xEA: case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA: break;
    case 0x80: case 0x82: case 0x89: case 0xC2: case 0xE2: pc++; break;
    case 0x04: case 0x44: case 0x64: rd(amZp()); break;
    case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4: rd(amZpX()); break;
    case 0x0C: rd(amAbs()); break;
    case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: rd(amAbsX()); break;

    // --- rozkazy nieudokumentowane ---
    // LAX
    case 0xA7: a = x = rd(amZp()); setNZ(a); break;
    case 0xB7: a = x = rd(amZpY()); setNZ(a); break;
    case 0xAF: a = x = rd(amAbs()); setNZ(a); break;
    case 0xBF: a = x = rd(amAbsY()); setNZ(a); break;
    case 0xA3: a = x = rd(amIndX()); setNZ(a); break;
    case 0xB3: a = x = rd(amIndY()); setNZ(a); break;
    case 0xAB: a = x = (a | 0xEE) & fetch(); setNZ(a); break;
    // SAX
    case 0x87: wr(amZp(), a & x); break;
    case 0x97: wr(amZpY(), a & x); break;
    case 0x8F: wr(amAbs(), a & x); break;
    case 0x83: wr(amIndX(), a & x); break;
    // SLO = ASL + ORA
    case 0x07: case 0x17: case 0x0F: case 0x1F: case 0x1B: case 0x03: case 0x13:
    // RLA = ROL + AND
    case 0x27: case 0x37: case 0x2F: case 0x3F: case 0x3B: case 0x23: case 0x33:
    // SRE = LSR + EOR
    case 0x47: case 0x57: case 0x4F: case 0x5F: case 0x5B: case 0x43: case 0x53:
    // RRA = ROR + ADC
    case 0x67: case 0x77: case 0x6F: case 0x7F: case 0x7B: case 0x63: case 0x73:
    // DCP = DEC + CMP
    case 0xC7: case 0xD7: case 0xCF: case 0xDF: case 0xDB: case 0xC3: case 0xD3:
    // ISC = INC + SBC
    case 0xE7: case 0xF7: case 0xEF: case 0xFF: case 0xFB: case 0xE3: case 0xF3: {
        switch (op & 0x1F) {
        case 0x07: ad = amZp(); break;
        case 0x17: ad = amZpX(); break;
        case 0x0F: ad = amAbs(); break;
        case 0x1F: ad = amAbsX(); break;
        case 0x1B: ad = amAbsY(); break;
        case 0x03: ad = amIndX(); break;
        default:   ad = amIndY(); break;
        }
        v = rd(ad);
        switch (op & 0xE0) {
        case 0x00: v = asl(v); a |= v; setNZ(a); break;
        case 0x20: v = rol(v); a &= v; setNZ(a); break;
        case 0x40: v = lsr(v); a ^= v; setNZ(a); break;
        case 0x60: v = ror(v); adc(v); break;
        case 0xC0: v--; cmp(a, v); break;
        default:   v++; sbc(v); break;
        }
        wr(ad, v);
        break;
    }
    case 0x0B: case 0x2B: a &= fetch(); setNZ(a); setFlag(FLAG_C, a & 0x80); break; // ANC
    case 0x4B: a &= fetch(); a = lsr(a); break;                                       // ALR
    case 0x6B: {                                                                      // ARR
        a &= fetch();
        a = uint8_t((a >> 1) | ((p & FLAG_C) ? 0x80 : 0));
        setNZ(a);
        setFlag(FLAG_C, a & 0x40);
        setFlag(FLAG_V, ((a >> 6) ^ (a >> 5)) & 1);
        break;
    }
    case 0xCB: {                                                                      // SBX/AXS
        unsigned t = (a & x) - fetch();
        setFlag(FLAG_C, t < 0x100);
        x = uint8_t(t); setNZ(x);
        break;
    }
    case 0xBB: v = rd(amAbsY()) & sp; a = x = sp = v; setNZ(v); break;               // LAS
    case 0x9B: case 0x9C: case 0x9E: case 0x9F: case 0x93: {                          // SHA/SHX/SHY/TAS
        uint16_t base = (op == 0x93) ? 0 : fetch16();
        if (op == 0x93) {
            uint8_t z = fetch();
            base = rd(z) | (uint16_t(rd(uint8_t(z + 1))) << 8);
        }
        uint8_t hi1 = uint8_t((base >> 8) + 1);
        switch (op) {
        case 0x9C: wr(uint16_t(base + x), y & hi1); break;
        case 0x9E: wr(uint16_t(base + y), x & hi1); break;
        case 0x9B: sp = a & x; wr(uint16_t(base + y), sp & hi1); break;
        default:   wr(uint16_t(base + y), a & x & hi1); break;
        }
        break;
    }
    // KIL/JAM - procesor zawiesza sie
    default:
        jammed = true;
        pc--;
        return 2;
    }
#undef LOAD
#undef STORE
#undef RMW
    return CYCLES[op];
}
