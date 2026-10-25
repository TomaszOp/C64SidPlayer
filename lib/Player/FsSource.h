/**
 * @file FsSource.h
 * @brief FsSource - common part of the sources based on the Arduino file system (fs::FS).
 *
 * LittleFS in flash memory and the SD card share the same interface.
 */
#pragma once
#include "SidSource.h"

/**
 * @brief SidSource implemented on top of an Arduino fs::FS (read and write).
 */
class FsSource : public SidSource {
public:
    /**
     * @brief Creates the source on a file system.
     * @param fs File system (e.g. LittleFS or SD).
     */
    explicit FsSource(fs::FS& fs) : fs_(fs) {}

    /**
     * @brief Lists subfolders and .sid files, skipping hidden/system entries.
     * @param dir        Folder path.
     * @param out        Receives the entries (cleared first).
     * @param maxEntries Maximum number of entries.
     * @return false if the folder does not exist.
     */
    bool list(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) override;
    /**
     * @brief Checks whether a path is an existing folder.
     * @param path Path.
     * @return true if it is a folder.
     */
    bool isDir(const String& path) override;
    /**
     * @brief Reads a whole file into a malloc buffer.
     * @param path   File path.
     * @param buf    Receives the buffer (nullptr on error).
     * @param len    Receives the length.
     * @param maxLen Maximum allowed size.
     * @return "" on success, otherwise an error description.
     */
    String read(const String& path, uint8_t*& buf, size_t& len, size_t maxLen) override;
    /** @brief File systems are writable. @return true. */
    bool canWrite() const override { return true; }
    /**
     * @brief Opens a file for writing (mode "w").
     * @param path File path.
     * @return Open file (invalid on failure).
     */
    fs::File openWrite(const String& path) override { return fs_.open(path, "w"); }
    /**
     * @brief Deletes a file.
     * @param path File path.
     * @return true if deleted.
     */
    bool remove(const String& path) override { return fs_.remove(path); }

protected:
    fs::FS& fs_;   ///< Underlying file system.
};

/**
 * @brief Flash memory (LittleFS). Note: the whole path may have at most ~63 characters.
 */
class FlashSource : public FsSource {
public:
    /** @brief Creates the source on LittleFS. */
    FlashSource();
    /**
     * @brief Mounts LittleFS (formats it if cfg.formatFsIfFailed and mounting fails).
     * @param cfg Player configuration.
     * @return true if mounted.
     */
    bool begin(const PlayerConfig& cfg) override;
    /** @brief Whether LittleFS is mounted. @return true if mounted. */
    bool ready() override { return mounted_; }
    /** @brief Error text. @return "" when mounted, otherwise the reason. */
    const char* error() const override { return mounted_ ? "" : "LittleFS nie zamontowany"; }
    /** @brief LittleFS path limit. @return 63. */
    size_t maxPathLength() const override { return 63; }
    /** @brief LittleFS capacity. @return Bytes (0 when not mounted). */
    uint64_t totalSpace() override;
    /** @brief LittleFS free space. @return Bytes (0 when not mounted). */
    uint64_t freeSpace() override;
private:
    bool mounted_ = false;   ///< LittleFS mounted successfully.
};

#if PLAYER_SOURCE_SD
/**
 * @brief SD card over SPI.
 *
 * Removing the card is detected on the next read,
 * and inserting it - on a later retry (every few seconds).
 */
class SdSource : public FsSource {
public:
    /** @brief Creates the source on the SD library. */
    SdSource();
    /**
     * @brief Remembers the configuration and tries to mount the card.
     * @param cfg Player configuration (SPI pins and frequency; must outlive the source).
     * @return true if a card was mounted.
     */
    bool begin(const PlayerConfig& cfg) override;
    /**
     * @brief Whether a card is mounted; retries mounting every SD_RETRY_MS.
     * @return true if mounted.
     */
    bool ready() override;
    /** @brief Error text. @return "" when mounted, otherwise "no SD card" message. */
    const char* error() const override { return mounted_ ? "" : "brak karty SD"; }
    /**
     * @brief Lists a folder; detects a removed card.
     * @param dir        Folder path.
     * @param out        Receives the entries.
     * @param maxEntries Maximum number of entries.
     * @return true on success.
     */
    bool list(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) override;
    /**
     * @brief Reads a file; detects a removed card.
     * @param path   File path.
     * @param buf    Receives the buffer.
     * @param len    Receives the length.
     * @param maxLen Maximum allowed size.
     * @return "" on success, otherwise an error description.
     */
    String read(const String& path, uint8_t*& buf, size_t& len, size_t maxLen) override;
    /** @brief Card capacity. @return Bytes (0 when not mounted). */
    uint64_t totalSpace() override;
    /** @brief Card free space. @return Bytes (0 when not mounted). */
    uint64_t freeSpace() override;
private:
    /** @brief Starts SPI (once) and mounts the card. @return true if mounted. */
    bool mount();
    /** @brief Marks the card as removed and unmounts it. */
    void lost();
    const PlayerConfig* cfg_ = nullptr;   ///< Configuration passed to begin().
    bool mounted_ = false;     ///< Card mounted.
    bool spiReady_ = false;    ///< SPI bus already started.
    uint32_t lastTry_ = 0;     ///< millis() of the last mount attempt.
};
#endif
