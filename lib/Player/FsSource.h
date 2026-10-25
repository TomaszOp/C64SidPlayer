// FsSource - wspolna czesc zrodel opartych na systemie plikow Arduino (fs::FS):
// LittleFS w pamieci flash i karta SD maja ten sam interfejs.
#pragma once
#include "SidSource.h"

class FsSource : public SidSource {
public:
    explicit FsSource(fs::FS& fs) : fs_(fs) {}

    bool list(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) override;
    bool isDir(const String& path) override;
    String read(const String& path, uint8_t*& buf, size_t& len, size_t maxLen) override;
    bool canWrite() const override { return true; }
    fs::File openWrite(const String& path) override { return fs_.open(path, "w"); }
    bool remove(const String& path) override { return fs_.remove(path); }

protected:
    fs::FS& fs_;
};

// Pamiec flash (LittleFS). Uwaga: cala sciezka moze miec najwyzej ~63 znaki.
class FlashSource : public FsSource {
public:
    FlashSource();
    bool begin(const PlayerConfig& cfg) override;
    bool ready() override { return mounted_; }
    const char* error() const override { return mounted_ ? "" : "LittleFS nie zamontowany"; }
    size_t maxPathLength() const override { return 63; }
    uint64_t totalSpace() override;
    uint64_t freeSpace() override;
private:
    bool mounted_ = false;
};

#if PLAYER_SOURCE_SD
// Karta SD przez SPI. Wyjecie karty jest wykrywane przy nastepnym odczycie,
// a wlozenie - przy kolejnej probie (co kilka sekund).
class SdSource : public FsSource {
public:
    SdSource();
    bool begin(const PlayerConfig& cfg) override;
    bool ready() override;
    const char* error() const override { return mounted_ ? "" : "brak karty SD"; }
    bool list(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) override;
    String read(const String& path, uint8_t*& buf, size_t& len, size_t maxLen) override;
    uint64_t totalSpace() override;
    uint64_t freeSpace() override;
private:
    bool mount();
    void lost();
    const PlayerConfig* cfg_ = nullptr;
    bool mounted_ = false;
    bool spiReady_ = false;
    uint32_t lastTry_ = 0;
};
#endif
