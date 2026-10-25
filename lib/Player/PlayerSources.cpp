// Lista zrodel plikow - tu wlaczasz zrodla i ustawiasz, co wolno uzytkownikowi.
#include "SidSource.h"
#include "FsSource.h"
#include "DeepSidSource.h"

const SourceInfo PLAYER_SOURCES[] = {
    //  typ                  id       opis            upload
    {SourceType::Flash,  "flash", "Pamiec flash", true},
#if PLAYER_SOURCE_SD
    {SourceType::SdCard, "sd",    "Karta SD",     true},
#endif
#if PLAYER_SOURCE_DEEPSID
    {SourceType::DeepSid, "deepsid", "DeepSid",   false},
#endif
};
const uint8_t PLAYER_SOURCE_COUNT = sizeof(PLAYER_SOURCES) / sizeof(PLAYER_SOURCES[0]);

SidSource* sourceImpl(SourceType type) {
    static FlashSource flash;
#if PLAYER_SOURCE_SD
    static SdSource sd;
#endif
#if PLAYER_SOURCE_DEEPSID
    static DeepSidSource deepsid;
#endif
    switch (type) {
    case SourceType::Flash: return &flash;
#if PLAYER_SOURCE_SD
    case SourceType::SdCard: return &sd;
#endif
#if PLAYER_SOURCE_DEEPSID
    case SourceType::DeepSid: return &deepsid;
#endif
    default: return nullptr;
    }
}

const SourceInfo* sourceInfo(SourceType type) {
    for (uint8_t i = 0; i < PLAYER_SOURCE_COUNT; i++)
        if (PLAYER_SOURCES[i].type == type) return &PLAYER_SOURCES[i];
    return nullptr;
}

const SourceInfo* sourceInfo(const String& id) {
    for (uint8_t i = 0; i < PLAYER_SOURCE_COUNT; i++)
        if (id == PLAYER_SOURCES[i].id || id == String(uint8_t(PLAYER_SOURCES[i].type)))
            return &PLAYER_SOURCES[i];
    return nullptr;
}

// ---------------------------------------------------------------- sciezki

namespace SourcePath {

String join(const String& dir, const String& name) {
    if (!dir.length() || dir == "/") return "/" + name;
    return dir + "/" + name;
}

String parent(const String& path) {
    int slash = path.lastIndexOf('/');
    return slash <= 0 ? String("/") : path.substring(0, slash);
}

String baseName(const String& path) {
    int slash = path.lastIndexOf('/');
    return slash < 0 ? path : path.substring(slash + 1);
}

bool valid(const String& path) {
    if (!path.startsWith("/")) return false;
    if (path.indexOf("//") >= 0 || path.indexOf('\\') >= 0) return false;
    if (path.length() > 1 && path.endsWith("/")) return false;
    // zaden element sciezki nie moze byc "." ani ".."
    String p = path + "/";
    return p.indexOf("/../") < 0 && p.indexOf("/./") < 0;
}

bool isSid(const String& name) {
    String l = name;
    l.toLowerCase();
    return l.endsWith(".sid");
}

}  // namespace SourcePath
