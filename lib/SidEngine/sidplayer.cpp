/**
 * @file sidplayer.cpp
 * @brief Implementation of SidPlayer: .sid loading, C64 memory map and play timing.
 */
#include "sidplayer.h"
#include <stdlib.h>
#include <string.h>
#include <initializer_list>

static const double PAL_CLOCK = 985248.0;      ///< PAL C64 clock in Hz.
static const double NTSC_CLOCK = 1022727.0;    ///< NTSC C64 clock in Hz.
static const uint32_t PAL_FRAME = 19656;     ///< PAL frame length in cycles: 312 lines * 63 cycles.
static const uint32_t NTSC_FRAME = 17095;    ///< NTSC frame length in cycles: 263 lines * 65 cycles.
static const uint16_t PAL_CIA_DEFAULT = 0x4025;    ///< Default CIA timer A latch on PAL (KERNAL value, ~60 Hz).
static const uint16_t NTSC_CIA_DEFAULT = 0x4295;   ///< Default CIA timer A latch on NTSC (KERNAL value, ~60 Hz).

/// Trap address: returning from a routine (RTS/RTI) sets PC here and ends the call.
static const uint16_t TRAP_ADDR = 0x0000;

/**
 * @brief Reads a big-endian 16-bit value (as stored in the .sid header).
 * @param p Pointer to the two bytes.
 * @return The value.
 */
static inline uint16_t be16(const uint8_t* p) { return uint16_t((p[0] << 8) | p[1]); }

SidPlayer::SidPlayer()
    : image(nullptr), err(""), cpu(this), modelOverride(-1), sampleRate(44100),
      clockHz(PAL_CLOCK), frameCycles(PAL_FRAME), cycleAcc(0), cyclesPerSample(1),
      cpuCycles(0), samplesPlayed(0), frameStartCycle(0), frameCpuBase(0), song(0), useCia(false), ciaLatch(PAL_CIA_DEFAULT) {
    mem = (uint8_t*)calloc(1, 0x10000);
    kernal = (uint8_t*)calloc(1, 0x2000);
    memset(io, 0, sizeof(io));
    memset(&hdr, 0, sizeof(hdr));
}

SidPlayer::~SidPlayer() {
    free(mem);
    free(kernal);
    free(image);
}

bool SidPlayer::load(const uint8_t* data, size_t len) {
    free(image);
    image = nullptr;
    song = 0;
    if (!mem || !kernal) { err = "brak pamieci"; return false; }
    if (len < 0x76) { err = "plik za krotki"; return false; }

    bool rsid;
    if (!memcmp(data, "PSID", 4)) rsid = false;
    else if (!memcmp(data, "RSID", 4)) rsid = true;
    else { err = "to nie jest plik PSID/RSID"; return false; }

    SidInfo h;
    memset(&h, 0, sizeof(h));
    h.rsid = rsid;
    h.version = be16(data + 4);
    uint16_t dataOffset = be16(data + 6);
    h.loadAddr = be16(data + 8);
    h.initAddr = be16(data + 0x0A);
    h.playAddr = be16(data + 0x0C);
    h.songs = be16(data + 0x0E);
    h.startSong = be16(data + 0x10);
    h.speed = (uint32_t(data[0x12]) << 24) | (uint32_t(data[0x13]) << 16) |
              (uint32_t(data[0x14]) << 8) | data[0x15];
    memcpy(h.name, data + 0x16, 32);
    memcpy(h.author, data + 0x36, 32);
    memcpy(h.released, data + 0x56, 32);
    h.ntsc = false;
    h.model = Sid::MOS6581;
    if (h.version >= 2 && len >= 0x7C) {
        uint16_t flags = be16(data + 0x76);
        if (rsid && (flags & 0x02)) { err = "utwor wymaga interpretera BASIC (nieobslugiwane)"; return false; }
        if (((flags >> 2) & 3) == 2) h.ntsc = true;
        if (((flags >> 4) & 3) == 2) h.model = Sid::MOS8580;
    }
    if (dataOffset >= len) { err = "niepoprawny offset danych"; return false; }

    const uint8_t* payload = data + dataOffset;
    size_t payloadLen = len - dataOffset;
    if (h.loadAddr == 0) {     // adres ladowania w pierwszych 2 bajtach danych (little endian)
        if (payloadLen < 2) { err = "brak danych"; return false; }
        h.loadAddr = uint16_t(payload[0] | (payload[1] << 8));
        payload += 2;
        payloadLen -= 2;
    }
    if (payloadLen > size_t(0x10000 - h.loadAddr)) payloadLen = 0x10000 - h.loadAddr;
    if (payloadLen == 0) { err = "brak danych"; return false; }
    if (h.initAddr == 0) h.initAddr = h.loadAddr;
    if (h.songs == 0) h.songs = 1;
    if (h.startSong == 0 || h.startSong > h.songs) h.startSong = 1;
    h.dataLen = uint16_t(payloadLen);

    image = (uint8_t*)malloc(payloadLen);
    if (!image) { err = "brak pamieci na dane"; return false; }
    memcpy(image, payload, payloadLen);
    hdr = h;
    err = "";
    return true;
}

void SidPlayer::installKernalStubs() {
    // Wypelnienie RTS: wywolania procedur KERNAL po prostu wracaja.
    memset(kernal, 0x60, 0x2000);
    auto put = [&](uint16_t addr, std::initializer_list<uint8_t> bytes) {
        uint16_t a = addr - 0xE000;
        for (uint8_t b : bytes) kernal[a++] = b;
    };
    // $FF48: wejscie IRQ -> zapis rejestrow i skok przez wektor $0314
    put(0xFF48, {0x48, 0x8A, 0x48, 0x98, 0x48, 0x6C, 0x14, 0x03});
    // $EA31: standardowa obsluga IRQ -> potwierdzenie CIA i powrot
    put(0xEA31, {0xAD, 0x0D, 0xDC, 0x4C, 0x81, 0xEA});
    put(0xEA7E, {0xAD, 0x0D, 0xDC});
    // $EA81: PLA TAY PLA TAX PLA RTI
    put(0xEA81, {0x68, 0xA8, 0x68, 0xAA, 0x68, 0x40});
    // $FE43 (NMI) / $FE47: RTI
    put(0xFE43, {0x40});
    put(0xFE47, {0x40});
    // wektory sprzetowe
    put(0xFFFA, {0x43, 0xFE, 0xE2, 0xFC, 0x48, 0xFF});
    // wektory RAM: IRQ -> $EA31, BRK -> $FE66, NMI -> $FE47
    mem[0x0314] = 0x31; mem[0x0315] = 0xEA;
    mem[0x0316] = 0x66; mem[0x0317] = 0xFE;
    mem[0x0318] = 0x47; mem[0x0319] = 0xFE;
}

bool SidPlayer::startSong(int n) {
    if (!image) { err = "brak wczytanego pliku"; return false; }
    if (n <= 0 || n > hdr.songs) n = hdr.startSong;
    song = n;

    memset(mem, 0, 0x10000);
    memset(io, 0, sizeof(io));
    installKernalStubs();
    memcpy(mem + hdr.loadAddr, image, hdr.dataLen);

    clockHz = hdr.ntsc ? NTSC_CLOCK : PAL_CLOCK;
    cyclesPerSample = clockHz / sampleRate;
    Sid::Model model = modelOverride >= 0 ? Sid::Model(modelOverride) : hdr.model;
    sid.init(clockHz, sampleRate, model);

    ciaLatch = hdr.ntsc ? NTSC_CIA_DEFAULT : PAL_CIA_DEFAULT;
    io[0xC04] = ciaLatch & 0xFF;
    io[0xC05] = ciaLatch >> 8;

    // Konfiguracja pamieci ($01) zgodnie ze specyfikacja PSID
    uint8_t bank = 0x37;
    if (!hdr.rsid) {
        if (hdr.initAddr >= 0xE000) bank = 0x35;
        else if (hdr.initAddr >= 0xD000) bank = 0x34;
        else if (hdr.initAddr >= 0xA000) bank = 0x36;
    }
    mem[0] = 0x2F;
    mem[1] = bank;

    cpu.reset();
    cpuCycles = 0;
    samplesPlayed = 0;
    cycleAcc = 0;
    frameStartCycle = 0;
    frameCpuBase = 0;
    // init moze nie wracac (RSID z petla glowna) - wtedy przerywamy po limicie cykli
    callRoutine(hdr.initAddr, uint8_t(n - 1), 4000000);

    if (hdr.rsid) {
        useCia = !(io[0x01A] & 0x01);      // brak przerwania rastra -> timer CIA
    } else {
        int bit = (n - 1) < 32 ? (n - 1) : 31;
        useCia = (hdr.speed >> bit) & 1;
    }
    updateFrameCycles();
    return true;
}

void SidPlayer::updateFrameCycles() {
    if (useCia) {
        uint16_t latch = ciaLatch ? ciaLatch : (hdr.ntsc ? NTSC_CIA_DEFAULT : PAL_CIA_DEFAULT);
        frameCycles = double(latch) + 1.0;
        if (frameCycles < 200) frameCycles = 200;   // ochrona przed absurdalnie szybkim timerem
    } else {
        frameCycles = hdr.ntsc ? NTSC_FRAME : PAL_FRAME;
    }
}

bool SidPlayer::runUntilReturn(uint32_t maxCycles) {
    uint32_t used = 0;
    while (cpu.pc != TRAP_ADDR && !cpu.jammed && used < maxCycles) {
        int c = cpu.step();
        used += c;
        cpuCycles += c;
    }
    return cpu.pc == TRAP_ADDR;
}

bool SidPlayer::callRoutine(uint16_t addr, uint8_t a, uint32_t maxCycles) {
    cpu.jammed = false;
    cpu.sp = 0xFF;
    cpu.a = a; cpu.x = 0; cpu.y = 0;
    cpu.p = Cpu6502::FLAG_U | Cpu6502::FLAG_I;
    uint16_t ret = TRAP_ADDR - 1;        // RTS doda 1 -> TRAP_ADDR
    cpu.push(ret >> 8);
    cpu.push(ret & 0xFF);
    cpu.pc = addr;
    return runUntilReturn(maxCycles);
}

bool SidPlayer::callIrq(uint32_t maxCycles) {
    cpu.jammed = false;
    cpu.sp = 0xFF;
    cpu.push(TRAP_ADDR >> 8);
    cpu.push(TRAP_ADDR & 0xFF);
    cpu.push(cpu.p & ~Cpu6502::FLAG_B);
    cpu.p |= Cpu6502::FLAG_I;
    cpu.pc = read(0xFFFE) | (uint16_t(read(0xFFFF)) << 8);
    return runUntilReturn(maxCycles);
}

void SidPlayer::playFrame() {
    if (hdr.playAddr) {
        callRoutine(hdr.playAddr, 0, 60000);
    } else {
        callIrq(60000);
    }
    if (hdr.rsid) useCia = !(io[0x01A] & 0x01);
    updateFrameCycles();
}

void SidPlayer::render(int16_t* out, size_t n, float gain) {
    if (!image || !song) { memset(out, 0, n * sizeof(int16_t)); return; }
    const float k = gain * 32767.0f;
#if SID_ENGINE == SID_ENGINE_REAL
    sid.setMuted(gain <= 0.0f);
#endif
    for (size_t i = 0; i < n; i++) {
        cycleAcc += cyclesPerSample;
        if (cycleAcc >= frameCycles) {
            cycleAcc -= frameCycles;
            frameStartCycle = double(samplesPlayed + i) * cyclesPerSample;
            frameCpuBase = cpuCycles;
            playFrame();
        }
        float s = sid.clock() * k;
        if (s > 32767.0f) s = 32767.0f;
        if (s < -32768.0f) s = -32768.0f;
        out[i] = int16_t(s);
    }
    samplesPlayed += n;
}

// ---------------- Magistrala / mapa pamieci C64 ----------------

uint8_t SidPlayer::read(uint16_t addr) {
    uint8_t port = mem[1];
    if (addr >= 0xE000) {
        if (port & 0x02) return kernal[addr - 0xE000];
    } else if (addr >= 0xD000 && addr < 0xE000) {
        if ((port & 0x03) && (port & 0x04)) return ioRead(addr);
    }
    return mem[addr];
}

void SidPlayer::write(uint16_t addr, uint8_t v) {
    if (addr >= 0xD000 && addr < 0xE000) {
        uint8_t port = mem[1];
        if ((port & 0x03) && (port & 0x04)) { ioWrite(addr, v); return; }
    }
    mem[addr] = v;
}

uint8_t SidPlayer::ioRead(uint16_t addr) {
    uint16_t off = addr - 0xD000;
    if (addr >= 0xD400 && addr < 0xD800) return sid.read(addr & 0x1F);

    uint32_t lineCycles = hdr.ntsc ? 65 : 63;
    uint32_t lines = hdr.ntsc ? 263 : 312;
    uint32_t raster = uint32_t((cpuCycles / lineCycles) % lines);
    switch (addr) {
    case 0xD011: return uint8_t((io[0x011] & 0x7F) | ((raster >> 1) & 0x80));
    case 0xD012: return uint8_t(raster);
    case 0xD019: return 0xF1;                 // zgloszone przerwanie rastra
    case 0xDC04: {                             // biezaca wartosc timera A
        uint32_t t = ciaLatch ? ciaLatch : 1;
        uint32_t cnt = t - uint32_t(cpuCycles % (t + 1));
        return uint8_t(cnt);
    }
    case 0xDC05: {
        uint32_t t = ciaLatch ? ciaLatch : 1;
        uint32_t cnt = t - uint32_t(cpuCycles % (t + 1));
        return uint8_t(cnt >> 8);
    }
    case 0xDC0D: return 0x81;                 // przerwanie od timera A
    case 0xDD0D: return 0x00;
    default: return io[off];
    }
}

void SidPlayer::ioWrite(uint16_t addr, uint8_t v) {
    uint16_t off = addr - 0xD000;
    if (addr >= 0xD400 && addr < 0xD800) {
#if SID_ENGINE == SID_ENGINE_REAL
        // prawdziwy uklad: zapis z czasem = poczatek ramki + cykle CPU od jej poczatku
        double d = double(cpuCycles - frameCpuBase);
        sid.setTime(frameStartCycle + (d < frameCycles ? d : frameCycles));
#endif
        sid.write(addr & 0x1F, v);
        return;
    }
    io[off] = v;
    switch (addr) {
    case 0xDC04: ciaLatch = (ciaLatch & 0xFF00) | v; break;
    case 0xDC05: ciaLatch = (ciaLatch & 0x00FF) | (uint16_t(v) << 8); break;
    default: break;
    }
}
