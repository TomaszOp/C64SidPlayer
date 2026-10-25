// Zrodla plikow .sid: pamiec flash (LittleFS), karta SD, portal DeepSID (internet).
//
// Kazde zrodlo to klasa pochodna SidSource. Opis zrodel (krotka nazwa, opis,
// czy wolno wgrywac pliki) jest w tablicy PLAYER_SOURCES w PlayerSources.cpp.
//
// Nowe zrodlo:
//   1. dopisz wartosc do enum SourceType (przed Count) - nawet jesli bedzie
//      wlaczane dyrektywa, wartosc zostaje, wiec zapisane ustawienia sa wazne,
//   2. napisz klase pochodna SidSource (dla systemu plikow: FsSource),
//   3. dopisz pozycje w PLAYER_SOURCES i w sourceImpl() w PlayerSources.cpp,
//   4. dodaj folder / plik tego zrodla w lib/PlayerMemory/PlayerSettings.def.
//
// Sciezki zawsze zaczynaja sie od "/" (katalog glowny zrodla), np. "/Hubbard/Commando.sid".
#pragma once
#include <Arduino.h>
#include <FS.h>
#include <vector>

struct PlayerConfig;

enum class SourceType : uint8_t {
    Flash = 0,      // LittleFS - zawsze dostepne
    SdCard,         // karta SD przez SPI (-DPLAYER_SOURCE_SD=1)
    DeepSid,        // portal DeepSID przez internet (-DPLAYER_SOURCE_DEEPSID=1)
    Count           // zawsze ostatnie
};
constexpr uint8_t SOURCE_TYPE_COUNT = uint8_t(SourceType::Count);

// Opis zrodla - tu ustawiasz, co wolno uzytkownikowi.
struct SourceInfo {
    SourceType type;
    const char* id;         // krotka nazwa w API / konsoli, np. "sd"
    const char* name;       // opis na stronie
    bool upload;            // czy wolno wgrywac pliki (do biezacego folderu)
};

struct SourceEntry {
    String name;            // sama nazwa, bez folderu
    size_t size;
    bool dir;               // folder
};

class SidSource {
public:
    virtual ~SidSource() {}
    virtual bool begin(const PlayerConfig& cfg) = 0;
    virtual bool ready() = 0;                       // czy mozna czytac (np. karta wlozona)
    virtual const char* error() const { return ""; }   // przyczyna, gdy ready() == false

    // Zawartosc folderu: foldery i pliki .sid (bez sortowania, bez plikow ukrytych).
    virtual bool list(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) = 0;
    virtual bool isDir(const String& path) = 0;
    // Wczytuje caly plik do pamieci (malloc), zwraca opis bledu albo "".
    virtual String read(const String& path, uint8_t*& buf, size_t& len, size_t maxLen) = 0;

    // Zapis - zrodla tylko do odczytu zostawiaja wersje domyslne.
    virtual bool canWrite() const { return false; }
    virtual fs::File openWrite(const String& path) { return fs::File(); }
    virtual bool remove(const String& path) { return false; }
    virtual size_t maxPathLength() const { return 255; }

    virtual uint64_t totalSpace() { return 0; }
    virtual uint64_t freeSpace() { return 0; }
};

// Tablica zrodel wkompilowanych w program (PlayerSources.cpp).
extern const SourceInfo PLAYER_SOURCES[];
extern const uint8_t PLAYER_SOURCE_COUNT;
const SourceInfo* sourceInfo(SourceType type);      // nullptr = zrodlo nie jest wkompilowane
const SourceInfo* sourceInfo(const String& id);     // po krotkiej nazwie albo numerze
SidSource* sourceImpl(SourceType type);             // obiekt zrodla albo nullptr

// Pomocnicze funkcje do sciezek.
namespace SourcePath {
String join(const String& dir, const String& name);  // "/a" + "b" -> "/a/b"
String parent(const String& path);                   // "/a/b" -> "/a", "/a" -> "/"
String baseName(const String& path);                 // "/a/b.sid" -> "b.sid"
bool valid(const String& path);                      // zaczyna sie od "/", bez "..", "//" i "\"
bool isSid(const String& name);
}
