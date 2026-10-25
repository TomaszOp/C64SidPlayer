/**
 * @file DeepSidSource.h
 * @brief DeepSidSource - .sid files from the DeepSID portal (https://deepsid.chordian.net),
 *        i.e. the HVSC collection downloaded over the internet. Enabled with -DPLAYER_SOURCE_DEEPSID=1.
 *
 * Folder listings come from the public DeepSID REST API (/api/v1/, no key):
 * @code
 *   ?folder=X   subfolders of X (the API returns all levels - only direct children are kept),
 *   ?file=X/    .sid files in folder X.
 * @endcode
 * The .sid file itself is downloaded like on the web page: /music/_High Voltage SID Collection/X/file.sid.
 *
 * Source paths are HVSC paths, e.g. "/MUSICIANS/H/Hubbard_Rob/Commando.sid".
 * HTTPS connection without certificate verification (public files, no passwords).
 * Note: the /MUSICIANS folder has thousands of subfolders on the lower levels, so its
 * listing takes several to over a dozen seconds to load.
 */
#pragma once
#include "SidSource.h"

#if PLAYER_SOURCE_DEEPSID
/**
 * @brief Read-only source of HVSC .sid files downloaded from DeepSID over HTTPS.
 */
class DeepSidSource : public SidSource {
public:
    /**
     * @brief Nothing to initialize.
     * @param cfg Unused.
     * @return Always true.
     */
    bool begin(const PlayerConfig& cfg) override { return true; }
    /**
     * @brief Ready when WiFi is connected and there was no recent network error.
     * @return true if ready.
     */
    bool ready() override;
    /** @brief Last error text. @return Error description. */
    const char* error() const override { return error_.c_str(); }

    /**
     * @brief Lists an HVSC folder: direct subfolders and .sid files.
     * @param dir        HVSC folder path.
     * @param out        Receives the entries (cleared first).
     * @param maxEntries Maximum number of entries.
     * @return false on a network or API error.
     */
    bool list(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) override;
    /**
     * @brief Checks whether a path is a folder (uses the cached listing of its parent when available).
     * @param path HVSC path.
     * @return true if it is a folder.
     */
    bool isDir(const String& path) override;
    /**
     * @brief Downloads a .sid file into a malloc buffer.
     * @param path   HVSC file path.
     * @param buf    Receives the buffer (nullptr on error).
     * @param len    Receives the length.
     * @param maxLen Maximum allowed size.
     * @return "" on success, otherwise an error description.
     */
    String read(const String& path, uint8_t*& buf, size_t& len, size_t maxLen) override;

private:
    /**
     * @brief Adds the direct subfolders of a folder (API ?folder=) to the list and caches them.
     * @param dir        HVSC folder path.
     * @param out        Entries are appended here.
     * @param maxEntries Maximum number of entries.
     * @return false on a network or API error.
     */
    bool listFolders(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries);
    /**
     * @brief Adds the .sid files of a folder (API ?file=) to the list.
     * @param dir        HVSC folder path.
     * @param out        Entries are appended here.
     * @param maxEntries Maximum number of entries.
     * @return false on a network or API error.
     */
    bool listFiles(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries);
    /**
     * @brief Records a network error (makes ready() false for a while).
     * @param why Error description.
     */
    void netFailed(const String& why);

    String error_ = "brak polaczenia z internetem";   ///< Last error description.
    bool netError_ = false;           ///< A network error happened recently.
    uint32_t netErrorAt_ = 0;         ///< millis() of the last network error.
    String cachedDir_;                ///< Last listed folder (for isDir).
    std::vector<String> cachedDirs_;  ///< Subfolder names of cachedDir_.
};
#endif
