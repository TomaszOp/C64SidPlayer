/**
 * @file sidplayer.h
 * @brief Player for .sid files (PSID and RSID formats).
 *
 * Loads the tune into the 64 KB memory of an emulated C64, calls the init routine
 * and then the play routine (or the IRQ handler) at the frame rate
 * (50 Hz PAL / 60 Hz NTSC) or at the CIA timer rate.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "cpu6502.h"
#include "sidchip.h"

/**
 * @brief Information read from the .sid file header.
 */
struct SidInfo {
    bool rsid;                 ///< true = RSID file, false = PSID.
    uint16_t version;          ///< Header version.
    uint16_t loadAddr, initAddr, playAddr;   ///< Load, init and play addresses (play = 0: tune installs its own IRQ).
    uint16_t songs, startSong; ///< Number of subtunes and the default subtune (1-based).
    uint32_t speed;            ///< Speed bits: bit n set = subtune n+1 uses CIA timing.
    char name[33], author[33], released[33];   ///< Title, author and release info (NUL-terminated).
    bool ntsc;                 ///< true = NTSC clock, false = PAL.
    Sid::Model model;          ///< SID model requested by the file.
    uint16_t dataLen;          ///< Length of the C64 data loaded at loadAddr.
};

/**
 * @brief Emulated C64 (CPU, memory map, CIA/VIC stubs, SID) that plays .sid files.
 *
 * It is also the memory Bus for its own CPU.
 */
class SidPlayer : public Bus {
public:
    /** @brief Allocates the 64 KB RAM and the KERNAL stub ROM. */
    SidPlayer();
    /** @brief Frees the memory and the loaded file image. */
    ~SidPlayer();

    /**
     * @brief Loads a .sid file from memory (the data is copied).
     * @param data File contents.
     * @param len  File length in bytes.
     * @return true on success; false on error, with the message in error().
     */
    bool load(const uint8_t* data, size_t len);
    /**
     * @brief Starts a subtune: resets the C64, copies the data and runs init.
     * @param song Subtune number (1-based); 0 or out of range = default subtune from the file.
     * @return true on success; false when no file is loaded (see error()).
     */
    bool startSong(int song);
    /**
     * @brief Generates mono 16-bit samples, calling the play routine as needed.
     *
     * Outputs silence when no file is loaded or no subtune has been started.
     * @param out  Output buffer.
     * @param n    Number of samples to generate.
     * @param gain Volume multiplier (1.0 = full scale; 0 also mutes a real SID chip).
     */
    void render(int16_t* out, size_t n, float gain = 1.0f);

    /**
     * @brief Sets the output sample rate (takes effect on the next startSong()).
     * @param sampleRate Sample rate in Hz.
     */
    void init(uint32_t sampleRate) { this->sampleRate = sampleRate; }
    /**
     * @brief Forces the SID model (takes effect on the next startSong()).
     * @param m -1 = automatic from the file, otherwise Sid::MOS6581 / Sid::MOS8580.
     */
    void setModelOverride(int m) { modelOverride = m; }

    /** @brief Whether a file is loaded. @return true if load() succeeded. */
    bool loaded() const { return image != nullptr; }
    /** @brief Header of the loaded file. @return Reference to the header info. */
    const SidInfo& info() const { return hdr; }
    /** @brief Last error message. @return Error text ("" when none). */
    const char* error() const { return err; }
    /** @brief Current subtune. @return Subtune number (1-based), 0 when none was started. */
    int currentSong() const { return song; }
    /** @brief Playback time of the current subtune. @return Milliseconds of rendered audio. */
    uint32_t playedMs() const { return uint32_t(samplesPlayed * 1000ULL / sampleRate); }
    /** @brief Timing source. @return true if the play routine is driven by the CIA timer, false = raster (VBI). */
    bool usesCiaTiming() const { return useCia; }
    /** @brief Play routine call rate. @return Calls per second in Hz. */
    float frameRateHz() const { return float(clockHz / frameCycles); }

    // Bus interface for the CPU
    /**
     * @brief CPU bus read with the C64 memory map ($01 banking, I/O, KERNAL stubs).
     * @param addr Address.
     * @return Byte read.
     */
    uint8_t read(uint16_t addr) override;
    /**
     * @brief CPU bus write (RAM or I/O depending on $01 banking).
     * @param addr  Address.
     * @param value Byte to write.
     */
    void write(uint16_t addr, uint8_t value) override;

private:
    /**
     * @brief Calls a subroutine with JSR semantics and runs until it returns.
     * @param addr      Routine address.
     * @param a         Value of the accumulator on entry.
     * @param maxCycles Cycle limit after which execution is abandoned.
     * @return true if the routine returned (RTS to the trap address) within the limit.
     */
    bool callRoutine(uint16_t addr, uint8_t a, uint32_t maxCycles);
    /**
     * @brief Simulates an IRQ through the vector at $FFFE and runs until RTI.
     * @param maxCycles Cycle limit.
     * @return true if the handler returned within the limit.
     */
    bool callIrq(uint32_t maxCycles);
    /**
     * @brief Executes instructions until PC reaches the trap address, the CPU jams or the limit is hit.
     * @param maxCycles Cycle limit.
     * @return true if PC reached the trap address.
     */
    bool runUntilReturn(uint32_t maxCycles);
    /** @brief Runs one play call (play routine or IRQ) and updates the frame timing. */
    void playFrame();
    /** @brief Recomputes frameCycles from the CIA latch or the PAL/NTSC frame length. */
    void updateFrameCycles();
    /** @brief Fills the KERNAL ROM with RTS stubs, minimal IRQ/NMI handlers and the vectors. */
    void installKernalStubs();
    /**
     * @brief Reads the I/O area $D000-$DFFF (SID, raster, CIA timer A, interrupt flags).
     * @param addr Address in $D000-$DFFF.
     * @return Byte read.
     */
    uint8_t ioRead(uint16_t addr);
    /**
     * @brief Writes the I/O area $D000-$DFFF (SID registers, CIA timer latch).
     * @param addr Address in $D000-$DFFF.
     * @param v    Byte to write.
     */
    void ioWrite(uint16_t addr, uint8_t v);

    uint8_t* mem;        ///< 64 KB RAM.
    uint8_t* kernal;     ///< 8 KB stub KERNAL ROM ($E000-$FFFF).
    uint8_t io[0x1000];  ///< Registers $D000-$DFFF.
    uint8_t* image;      ///< Copy of the file data (for restarting the tune).
    SidInfo hdr;         ///< Header of the loaded file.
    const char* err;     ///< Last error message.

    Cpu6502 cpu;         ///< Emulated 6510.
    SidChip sid;         ///< SID engine selected by SID_ENGINE.
    int modelOverride;   ///< -1 = model from the file, otherwise forced Sid::Model.
    uint32_t sampleRate; ///< Output sample rate in Hz.
    double clockHz;      ///< C64 clock (PAL or NTSC) in Hz.
    double frameCycles;  ///< CPU cycles between play calls.
    double cycleAcc;     ///< Cycles accumulated since the last play call.
    double cyclesPerSample;   ///< CPU cycles per output sample.
    uint64_t cpuCycles;  ///< Total CPU cycles executed since the subtune started.
    uint64_t samplesPlayed;   ///< Samples rendered since the subtune started.
    double frameStartCycle;   ///< Time of the current frame call (in cycles since the subtune start).
    uint64_t frameCpuBase;    ///< cpuCycles at the start of the frame.
    int song;            ///< Current subtune (1-based), 0 = none.
    bool useCia;         ///< true = CIA timer timing, false = raster (VBI).
    uint16_t ciaLatch;   ///< CIA 1 timer A latch value.
};
