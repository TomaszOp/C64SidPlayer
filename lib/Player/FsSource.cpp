/**
 * @file FsSource.cpp
 * @brief Implementation of FsSource, FlashSource (LittleFS) and SdSource (SD card over SPI).
 */
#include "FsSource.h"
#include "Player.h"
#include <LittleFS.h>
#if PLAYER_SOURCE_SD
#include <SPI.h>
#include <SD.h>
#endif

// ---------------------------------------------------------------- FsSource

bool FsSource::list(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) {
    out.clear();
    File d = fs_.open(dir);
    if (!d || !d.isDirectory()) return false;
    for (File f = d.openNextFile(); f && out.size() < maxEntries; f = d.openNextFile()) {
        String name = SourcePath::baseName(f.name());
        // pliki ukryte i systemowe (np. "._plik.sid" z macOS, "System Volume Information")
        if (!name.length() || name[0] == '.' || name == "System Volume Information") continue;
        bool isDir = f.isDirectory();
        if (!isDir && !SourcePath::isSid(name)) continue;
        out.push_back({name, isDir ? 0 : size_t(f.size()), isDir});
    }
    return true;
}

bool FsSource::isDir(const String& path) {
    File d = fs_.open(path);
    return d && d.isDirectory();
}

bool FsSource::removeDir(const String& path) {
    // collect the names first and close the folder - deleting while iterating is not safe
    std::vector<String> files, dirs;
    {
        File d = fs_.open(path);
        if (!d || !d.isDirectory()) return false;
        for (File f = d.openNextFile(); f; f = d.openNextFile()) {
            String full = SourcePath::join(path, SourcePath::baseName(f.name()));
            (f.isDirectory() ? dirs : files).push_back(full);
        }
    }
    bool ok = true;
    for (const String& f : files) ok = fs_.remove(f) && ok;
    for (const String& d : dirs) ok = removeDir(d) && ok;
    return fs_.rmdir(path) && ok;
}

String FsSource::read(const String& path, uint8_t*& buf, size_t& len, size_t maxLen) {
    buf = nullptr;
    File f = fs_.open(path, "r");
    if (!f || f.isDirectory()) return "nie ma pliku " + path;
    len = f.size();
    if (len > maxLen) return "plik za duzy";
    buf = (uint8_t*)malloc(len ? len : 1);
    if (!buf) return "brak pamieci";
    if (f.read(buf, len) != len) { free(buf); buf = nullptr; return "blad odczytu " + path; }
    return "";
}

// ---------------------------------------------------------------- flash

FlashSource::FlashSource() : FsSource(LittleFS) {}

bool FlashSource::begin(const PlayerConfig& cfg) {
    mounted_ = LittleFS.begin(cfg.formatFsIfFailed);
    return mounted_;
}

uint64_t FlashSource::totalSpace() { return mounted_ ? LittleFS.totalBytes() : 0; }
uint64_t FlashSource::freeSpace() { return mounted_ ? LittleFS.totalBytes() - LittleFS.usedBytes() : 0; }

// ---------------------------------------------------------------- karta SD

#if PLAYER_SOURCE_SD
static const uint32_t SD_RETRY_MS = 3000;   ///< Interval between SD card mount retries in ms.

SdSource::SdSource() : FsSource(SD) {}

bool SdSource::begin(const PlayerConfig& cfg) {
    cfg_ = &cfg;
    return mount();
}

bool SdSource::mount() {
    lastTry_ = millis();
    if (!spiReady_) {
        SPI.begin(cfg_->sdSck, cfg_->sdMiso, cfg_->sdMosi, cfg_->sdCs);
        spiReady_ = true;
    }
    mounted_ = SD.begin(cfg_->sdCs, SPI, cfg_->sdFreq) && SD.cardType() != CARD_NONE;
    if (!mounted_) SD.end();
    return mounted_;
}

void SdSource::lost() {
    SD.end();
    mounted_ = false;
    lastTry_ = millis();
}

bool SdSource::ready() {
    if (!mounted_ && cfg_ && millis() - lastTry_ > SD_RETRY_MS) mount();
    return mounted_;
}

bool SdSource::list(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) {
    if (!ready()) { out.clear(); return false; }
    if (FsSource::list(dir, out, maxEntries)) return true;
    if (!SD.open("/")) lost();          // nie da sie otworzyc nawet katalogu glownego - karta wyjeta
    return false;
}

String SdSource::read(const String& path, uint8_t*& buf, size_t& len, size_t maxLen) {
    if (!ready()) { buf = nullptr; return error(); }
    String e = FsSource::read(path, buf, len, maxLen);
    if (e.length() && !SD.open("/")) { lost(); return error(); }
    return e;
}

uint64_t SdSource::totalSpace() { return mounted_ ? SD.totalBytes() : 0; }
uint64_t SdSource::freeSpace() { return mounted_ ? SD.totalBytes() - SD.usedBytes() : 0; }
#endif
