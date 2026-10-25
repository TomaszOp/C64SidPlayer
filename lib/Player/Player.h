// Player - odtwarzacz muzyki z Commodore 64 (pliki .sid) dla ESP32.
//
// Biblioteka laczy emulacje C64/SID (SidEngine) z wyjsciem audio I2S
// i zrodlami plikow (SidSource.h): pamiec flash (LittleFS), karta SD...
// Kazde zrodlo ma wlasny biezacy folder. Dzwiek generuje osobne zadanie
// FreeRTOS uruchamiane w begin(). Metody plikow i odtwarzania wywoluj z jednego
// zadania (np. loop()); glosnosc, pauza i status sa bezpieczne z kazdego.
//
// Metody zwracajace String zwracaja pusty napis przy powodzeniu,
// a w razie bledu jego opis.
//
//   Player player;
//   player.begin();               // I2S + zrodla plikow + zadanie audio
//   player.setSource(SourceType::SdCard);
//   player.cd("Hubbard");         // folder biezacego zrodla, ".." = poziom wyzej
//   player.play("Commando.sid");  // albo playIndex(1), albo pelna sciezka "/Hubbard/Commando.sid"
//   player.pause(); player.resume(); player.stop();
//   player.setVolume(70); player.setMute(true);
#pragma once
#include <Arduino.h>
#include <FS.h>
#include <vector>
#include "sidplayer.h"
#include "SidSource.h"

struct PlayerConfig {
    uint32_t sampleRate = 44100;
    int pinBck = 26;              // zewnetrzny DAC I2S (PCM5102A, MAX98357A...)
    int pinWs = 25;
    int pinData = 22;
    uint8_t volume = 80;          // 0..100
    int audioCore = 1;            // rdzen dla zadania audio
    int audioPriority = 5;
    bool formatFsIfFailed = true; // sformatuj LittleFS, gdy nie da sie zamontowac
    // karta SD (SPI), uzywana gdy wlaczone -DPLAYER_SOURCE_SD=1
    int sdCs = 5, sdSck = 18, sdMiso = 19, sdMosi = 23;
    uint32_t sdFreq = 20000000;
};
// Wbudowany DAC klasycznego ESP32 (GPIO25/26) wlacza sie flaga -DSID_OUTPUT_INTERNAL_DAC.

enum class PlayerState : uint8_t { Stopped, Playing, Paused };

// Pozycja listy biezacego folderu: najpierw foldery, potem pliki, alfabetycznie.
typedef SourceEntry PlayerFile;

struct PlayerStatus {
    PlayerState state;
    bool loaded;                  // czy jakis plik jest wczytany
    String file;                  // pelna sciezka biezacego pliku, np. "/Hubbard/Commando.sid"
    SourceType fileSource;        // zrodlo biezacego pliku
    SourceType source;            // aktywne zrodlo (lista, next/prev, upload)
    String dir;                   // biezacy folder aktywnego zrodla
    String title, author, released;
    int song, songs;              // biezacy podutwor / liczba podutworow
    uint32_t ms;                  // czas odtwarzania podutworu
    bool rsid, ntsc;
    int sidModel;                 // model z pliku: 0 = 6581, 1 = 8580
    float frameHz;                // czestotliwosc wywolan procedury play
    uint8_t volume;
    bool muted;
    int modelSetting;             // -1 auto, 0 = 6581, 1 = 8580
    bool testTone;
    unsigned cpuLoad;             // obciazenie zadania audio w %
    const char* engine;           // nazwa silnika emulacji SID
};

class Player {
public:
    Player();

    bool begin(const PlayerConfig& cfg = PlayerConfig());
    bool audioOk() const { return i2sError_ == 0; }
    const char* audioError() const;
    const PlayerConfig& config() const { return cfg_; }

    // --- zrodla plikow ---
    SourceType source() const { return source_; }
    String setSource(SourceType type);            // blad, gdy zrodlo niewkompilowane
    const SourceInfo& sourceInfo() const;         // opis aktywnego zrodla
    SidSource& activeSource() { return *sources_[uint8_t(source_)]; }
    bool sourceReady();                           // np. czy karta SD jest wlozona
    String sourceError();

    // --- foldery (kazde zrodlo pamieta swoj) ---
    String dir() const { return dir_[uint8_t(source_)]; }
    String dir(SourceType type) const { return dir_[uint8_t(type)]; }
    String cd(const String& dir);                 // "x", "/a/b", ".." albo "/"
    void setDir(SourceType type, const String& dir);   // bez sprawdzania - do odtworzenia ustawien

    // --- pliki w biezacym folderze ---
    const std::vector<PlayerFile>& files();       // foldery + pliki .sid
    void refreshFiles();                          // przeczytaj folder ponownie przy nastepnym files()
    bool filesTruncated() const { return truncated_; }
    int fileCount();                              // tylko pliki, bez folderow
    String fileName(int index);                   // index od 1, tylko pliki
    int fileIndex(const String& nameOrPath);      // 0 = nie ma w biezacym folderze
    String path(const String& nameOrPath) const;  // nazwa -> pelna sciezka w biezacym folderze
    bool removeFile(const String& nameOrPath);
    uint64_t freeSpace();
    uint64_t totalSpace();
    bool uploadAllowed();
    bool deleteAllowed();                         // czy zrodlo pozwala usuwac pliki
    // Sciezka dla wgrywanego pliku w biezacym folderze; zwraca blad albo "".
    String prepareUpload(const String& fileName, String& path);
    fs::File openWrite(const String& path);
    bool removePath(const String& path);
    // Zamienia dowolna nazwe na bezpieczna nazwe pliku .sid ("" = niedozwolona).
    static String sanitizeName(const String& name);
    static const size_t MAX_FILE_SIZE = 0x10000 + 0x7C + 2;
    static const size_t MAX_LIST = 1000;          // najwiecej pozycji w jednym folderze

    // --- odtwarzanie ---
    String play(const String& nameOrPath, int song = 0);   // song 0 = domyslny z pliku
    String playIndex(int index, int song = 0);
    String nextFile();                                // tylko w biezacym folderze
    String prevFile();
    void stop();
    void pause();
    void resume();                                    // tez wznawia po stop()
    void togglePause();
    String setSong(int song);
    String nextSong();
    String prevSong();
    String restartSong();

    // --- dzwiek ---
    void setVolume(int percent);
    int volume() const { return volume_; }
    void volumeUp(int step = 10) { setVolume(volume_ + step); }
    void volumeDown(int step = 10) { setVolume(volume_ - step); }
    void setMute(bool on) { muted_ = on; }
    void toggleMute() { muted_ = !muted_; }
    bool muted() const { return muted_; }
    String setModel(int model);                       // -1 auto, 0 = 6581, 1 = 8580
    int model() const { return modelSetting_; }
    void setTestTone(bool on) { testTone_ = on; }     // sinus 440 Hz do testu wzmacniacza
    bool testTone() const { return testTone_; }

    // --- stan ---
    PlayerStatus status();
    PlayerState state() const { return state_; }
    bool isPlaying() const { return state_ == PlayerState::Playing; }
    unsigned cpuLoad() const;

private:
    static void audioTaskEntry(void* arg);
    void audioTask();
    bool setupI2S();
    String loadAndStart(const String& path, int song);
    String stepFile(int delta);
    int dirCount();
    void lock();
    void unlock();

    PlayerConfig cfg_;
    SidPlayer sid_;
    void* mutex_;
    volatile PlayerState state_;
    volatile bool muted_;
    volatile bool testTone_;
    volatile int volume_;
    volatile uint32_t renderUs_;
    int modelSetting_;
    int i2sError_;
    String file_;
    SourceType fileSource_;
    SidSource* sources_[SOURCE_TYPE_COUNT];
    SourceType source_;
    String dir_[SOURCE_TYPE_COUNT];
    std::vector<PlayerFile> list_;
    bool listValid_;
    bool truncated_;
};
