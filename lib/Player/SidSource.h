/**
 * @file SidSource.h
 * @brief Sources of .sid files: flash memory (LittleFS), SD card, DeepSID portal (internet).
 *
 * Every source is a class derived from SidSource. The source descriptions (short name, description,
 * whether writing is allowed) are in the PLAYER_SOURCES table in PlayerSources.cpp.
 *
 * Adding a new source:
 *   1. add a value to enum SourceType (before Count) - even if it is enabled by
 *      a define, the value stays, so saved settings remain valid,
 *   2. write a class derived from SidSource (for a file system: FsSource),
 *   3. add an entry to PLAYER_SOURCES and to sourceImpl() in PlayerSources.cpp,
 *   4. add the folder / file settings of this source to lib/PlayerMemory/PlayerSettings.def.
 *
 * Paths always start with "/" (the root of the source), e.g. "/Hubbard/Commando.sid".
 */
#pragma once
#include <Arduino.h>
#include <FS.h>
#include <vector>

struct PlayerConfig;

/** @brief Kind of file source; the numeric values are stored in the settings. */
enum class SourceType : uint8_t {
    Flash = 0,      ///< LittleFS - always available.
    SdCard,         ///< SD card over SPI (-DPLAYER_SOURCE_SD=1).
    DeepSid,        ///< DeepSID portal over the internet (-DPLAYER_SOURCE_DEEPSID=1).
    Count           ///< Number of source types (always last).
};
/// Number of SourceType values (size of per-source arrays).
constexpr uint8_t SOURCE_TYPE_COUNT = uint8_t(SourceType::Count);

/** @brief Description of a source - this is where you set what the user is allowed to do. */
struct SourceInfo {
    SourceType type;        ///< Source type.
    const char* id;         ///< Short name used in the API / console, e.g. "sd".
    const char* name;       ///< Description shown on the web page.
    bool upload;            ///< Whether writing is allowed: uploading/deleting files, creating/deleting folders.
};

/** @brief One entry of a folder listing. */
struct SourceEntry {
    String name;            ///< Name only, without the folder.
    size_t size;            ///< File size in bytes (0 for folders).
    bool dir;               ///< true = folder.
};

/**
 * @brief Abstract source of .sid files.
 */
class SidSource {
public:
    /** @brief Virtual destructor. */
    virtual ~SidSource() {}
    /**
     * @brief Initializes the source (mounts the file system etc.).
     * @param cfg Player configuration (pins, options).
     * @return true if the source is usable.
     */
    virtual bool begin(const PlayerConfig& cfg) = 0;
    /**
     * @brief Checks whether the source can be read (e.g. card inserted).
     * @return true if ready.
     */
    virtual bool ready() = 0;
    /**
     * @brief Reason why ready() returns false.
     * @return Error text, "" when none.
     */
    virtual const char* error() const { return ""; }

    /**
     * @brief Lists a folder: subfolders and .sid files (unsorted, without hidden files).
     * @param dir        Folder path.
     * @param out        Receives the entries (cleared first).
     * @param maxEntries Maximum number of entries.
     * @return true on success, false if the folder cannot be read.
     */
    virtual bool list(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) = 0;
    /**
     * @brief Checks whether a path is an existing folder.
     * @param path Folder path.
     * @return true if it is a folder.
     */
    virtual bool isDir(const String& path) = 0;
    /**
     * @brief Reads a whole file into memory allocated with malloc (the caller frees it).
     * @param path   File path.
     * @param buf    Receives the buffer (nullptr on error).
     * @param len    Receives the file length.
     * @param maxLen Maximum allowed file size.
     * @return "" on success, otherwise an error description.
     */
    virtual String read(const String& path, uint8_t*& buf, size_t& len, size_t maxLen) = 0;

    // Writing - read-only sources keep the default implementations.
    /** @brief Whether the source supports writing. @return false by default (read-only source). */
    virtual bool canWrite() const { return false; }
    /**
     * @brief Opens a file for writing (overwriting it).
     * @param path File path.
     * @return Open file, or an invalid File when not supported.
     */
    virtual fs::File openWrite(const String& path) { return fs::File(); }
    /**
     * @brief Deletes a file.
     * @param path File path.
     * @return true if deleted.
     */
    virtual bool remove(const String& path) { return false; }
    /**
     * @brief Creates a folder.
     * @param path Folder path.
     * @return true if created.
     */
    virtual bool makeDir(const String& path) { return false; }
    /**
     * @brief Deletes a folder together with everything inside it.
     * @param path Folder path.
     * @return true if deleted.
     */
    virtual bool removeDir(const String& path) { return false; }
    /** @brief Maximum full path length. @return Number of characters. */
    virtual size_t maxPathLength() const { return 255; }

    /** @brief Capacity of the storage. @return Bytes, 0 when unknown. */
    virtual uint64_t totalSpace() { return 0; }
    /** @brief Free space of the storage. @return Bytes, 0 when unknown. */
    virtual uint64_t freeSpace() { return 0; }
};

/// Sources compiled into the program (PlayerSources.cpp).
extern const SourceInfo PLAYER_SOURCES[];
/// Number of entries in PLAYER_SOURCES.
extern const uint8_t PLAYER_SOURCE_COUNT;
/**
 * @brief Finds the description of a source.
 * @param type Source type.
 * @return Pointer into PLAYER_SOURCES, nullptr = the source is not compiled in.
 */
const SourceInfo* sourceInfo(SourceType type);
/**
 * @brief Finds the description of a source by its short name or number.
 * @param id Short name (e.g. "sd") or the SourceType number as text.
 * @return Pointer into PLAYER_SOURCES, or nullptr.
 */
const SourceInfo* sourceInfo(const String& id);
/**
 * @brief Returns the (static) object implementing a source.
 * @param type Source type.
 * @return Source object, or nullptr when not compiled in.
 */
SidSource* sourceImpl(SourceType type);

/** @brief Path helper functions. */
namespace SourcePath {
/**
 * @brief Joins a folder and a name: "/a" + "b" -> "/a/b".
 * @param dir  Folder.
 * @param name Name in the folder.
 * @return Full path.
 */
String join(const String& dir, const String& name);
/**
 * @brief Parent folder: "/a/b" -> "/a", "/a" -> "/".
 * @param path Path.
 * @return Parent folder path.
 */
String parent(const String& path);
/**
 * @brief Last path element: "/a/b.sid" -> "b.sid".
 * @param path Path.
 * @return Name without the folder.
 */
String baseName(const String& path);
/**
 * @brief Validates a path: starts with "/", no "." / ".." elements, no "//", no "\" and no trailing "/".
 * @param path Path.
 * @return true if valid.
 */
bool valid(const String& path);
/**
 * @brief Checks for the .sid extension (case-insensitive).
 * @param name File name or path.
 * @return true if it ends with ".sid".
 */
bool isSid(const String& name);
}
