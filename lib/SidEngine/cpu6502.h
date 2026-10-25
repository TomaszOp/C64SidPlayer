/**
 * @file cpu6502.h
 * @brief Emulator of the MOS 6502/6510 processor (the Commodore 64 CPU).
 *
 * Supports all documented instructions, decimal mode (BCD)
 * and the most commonly used undocumented instructions (LAX, SAX, DCP, ISC...).
 */
#pragma once
#include <stdint.h>

/**
 * @brief Abstract memory bus the CPU reads from and writes to.
 */
class Bus {
public:
    /** @brief Virtual destructor. */
    virtual ~Bus() {}
    /**
     * @brief Reads one byte from the bus.
     * @param addr 16-bit address.
     * @return Byte at that address.
     */
    virtual uint8_t read(uint16_t addr) = 0;
    /**
     * @brief Writes one byte to the bus.
     * @param addr  16-bit address.
     * @param value Byte to write.
     */
    virtual void write(uint16_t addr, uint8_t value) = 0;
};

/**
 * @brief MOS 6502/6510 CPU core executing one instruction per step().
 */
class Cpu6502 {
public:
    /** @brief Bits of the processor status register P. */
    enum Flag : uint8_t {
        FLAG_C = 0x01, FLAG_Z = 0x02, FLAG_I = 0x04, FLAG_D = 0x08,
        FLAG_B = 0x10, FLAG_U = 0x20, FLAG_V = 0x40, FLAG_N = 0x80
    };

    /**
     * @brief Creates the CPU attached to a bus and resets it.
     * @param bus Memory bus used for all reads and writes (not owned).
     */
    explicit Cpu6502(Bus* bus) : bus(bus) { reset(); }

    /** @brief Resets registers: PC = 0, A = X = Y = 0, SP = $FD, P = U|I, clears the jammed state. */
    void reset();
    /**
     * @brief Executes one instruction.
     * @return Base number of cycles for the opcode (without page-crossing / branch penalties);
     *         2 when the CPU is jammed.
     */
    int step();

    /**
     * @brief Pushes a byte onto the stack (page $01).
     * @param v Byte to push.
     */
    void push(uint8_t v) { bus->write(0x0100 | sp, v); sp--; }
    /**
     * @brief Pulls a byte from the stack (page $01).
     * @return The pulled byte.
     */
    uint8_t pull() { sp++; return bus->read(0x0100 | sp); }

    uint16_t pc;                ///< Program counter.
    uint8_t a, x, y, sp, p;     ///< Accumulator, index registers X/Y, stack pointer, status register.
    bool jammed;   ///< The CPU executed a KIL/JAM instruction and is halted.

private:
    Bus* bus;   ///< Memory bus (not owned).

    /**
     * @brief Reads a byte from the bus.
     * @param addr Address.
     * @return Byte read.
     */
    inline uint8_t rd(uint16_t addr) { return bus->read(addr); }
    /**
     * @brief Writes a byte to the bus.
     * @param addr Address.
     * @param v    Byte to write.
     */
    inline void wr(uint16_t addr, uint8_t v) { bus->write(addr, v); }
    /**
     * @brief Reads the byte at PC and increments PC.
     * @return Fetched byte.
     */
    inline uint8_t fetch() { return rd(pc++); }
    /**
     * @brief Reads a little-endian 16-bit word at PC and advances PC by 2.
     * @return Fetched word.
     */
    inline uint16_t fetch16() { uint16_t lo = fetch(); return lo | (uint16_t(fetch()) << 8); }
    /**
     * @brief Sets the N and Z flags from a value.
     * @param v Result value.
     */
    inline void setNZ(uint8_t v) { p = (p & ~(FLAG_N | FLAG_Z)) | (v & 0x80) | (v ? 0 : FLAG_Z); }
    /**
     * @brief Sets or clears status flags.
     * @param f  Flag mask (Flag values).
     * @param on true = set, false = clear.
     */
    inline void setFlag(uint8_t f, bool on) { p = on ? (p | f) : (p & ~f); }

    // Addressing modes - return the effective address
    /** @brief Zero page addressing. @return Effective address. */
    inline uint16_t amZp() { return fetch(); }
    /** @brief Zero page,X addressing (wraps within page 0). @return Effective address. */
    inline uint16_t amZpX() { return uint8_t(fetch() + x); }
    /** @brief Zero page,Y addressing (wraps within page 0). @return Effective address. */
    inline uint16_t amZpY() { return uint8_t(fetch() + y); }
    /** @brief Absolute addressing. @return Effective address. */
    inline uint16_t amAbs() { return fetch16(); }
    /** @brief Absolute,X addressing. @return Effective address. */
    inline uint16_t amAbsX() { return uint16_t(fetch16() + x); }
    /** @brief Absolute,Y addressing. @return Effective address. */
    inline uint16_t amAbsY() { return uint16_t(fetch16() + y); }
    /** @brief Indexed indirect (zp,X) addressing. @return Effective address. */
    inline uint16_t amIndX() {
        uint8_t z = fetch() + x;
        return rd(z) | (uint16_t(rd(uint8_t(z + 1))) << 8);
    }
    /** @brief Indirect indexed (zp),Y addressing. @return Effective address. */
    inline uint16_t amIndY() {
        uint8_t z = fetch();
        uint16_t base = rd(z) | (uint16_t(rd(uint8_t(z + 1))) << 8);
        return uint16_t(base + y);
    }

    /**
     * @brief ADC: adds a value and carry to A (binary or BCD mode).
     * @param v Operand.
     */
    void adc(uint8_t v);
    /**
     * @brief SBC: subtracts a value and borrow from A (binary or BCD mode).
     * @param v Operand.
     */
    void sbc(uint8_t v);
    /**
     * @brief Compares a register with a value (CMP/CPX/CPY), setting C, N and Z.
     * @param reg Register value.
     * @param v   Operand.
     */
    void cmp(uint8_t reg, uint8_t v);
    /**
     * @brief Arithmetic shift left; bit 7 goes to C.
     * @param v Operand.
     * @return Shifted value.
     */
    uint8_t asl(uint8_t v);
    /**
     * @brief Logical shift right; bit 0 goes to C.
     * @param v Operand.
     * @return Shifted value.
     */
    uint8_t lsr(uint8_t v);
    /**
     * @brief Rotate left through carry.
     * @param v Operand.
     * @return Rotated value.
     */
    uint8_t rol(uint8_t v);
    /**
     * @brief Rotate right through carry.
     * @param v Operand.
     * @return Rotated value.
     */
    uint8_t ror(uint8_t v);
    /**
     * @brief Fetches a relative offset and jumps when the condition is true.
     * @param cond Branch condition.
     */
    void branch(bool cond);
};
