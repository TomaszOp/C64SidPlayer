// Player - odtwarzacz muzyki z Commodore 64 (pliki .sid) dla ESP32.
//
// Biblioteka laczy emulacje C64/SID (SidEngine) z wyjsciem audio I2S
// i plikami zapisanymi w pamieci flash (LittleFS). Wszystkie metody sa
// bezpieczne do wywolania z dowolnego zadania - dzwiek generuje osobne
// zadanie FreeRTOS uruchamiane w begin().
//
// Metody zwracajace String zwracaja pusty napis przy powodzeniu,
// a w razie bledu jego opis.
//
//   Player player;
//   player.begin();               // I2S + LittleFS + zadanie audio
//   player.play("Commando.sid");  // albo playIndex(1)
//   player.pause(); player.resume(); player.stop();
//   player.setVolume(70); player.setMute(true);
#pragma once
#include <Arduino.h>
#include <FS.h>
#include <vector>
#include "sidplayer.h"

struct PlayerConfig {
    uint32_t sampleRate = 44100;
    int pinBck = 26;              // zewnetrzny DAC I2S (PCM5102A, MAX98357A...)
    int pinWs = 25;
    int pinData = 22;
    uint8_t volume = 80;          // 0..100
    int audioCore = 1;            // rdzen dla zadania audio
    int audioPriority = 5;
    bool formatFsIfFailed = true; // sformatuj LittleFS, gdy nie da sie zamontowac
};
// Wbudowany DAC klasycznego ESP32 (GPIO25/26) wlacza sie flaga -DSID_OUTPUT_INTERNAL_DAC.

enum class PlayerState : uint8_t { Stopped, Playing, Paused };

struct PlayerFile {
    String name;                  // nazwa bez "/" na poczatku
    size_t size;
};

struct PlayerStatus {
    PlayerState state;
    bool loaded;                  // czy jakis plik jest wczytany
    String file;                  // nazwa biezacego pliku
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

    // --- pliki w pamieci flash ---
    std::vector<PlayerFile> files();          // posortowane alfabetycznie
    int fileCount();
    String fileName(int index);               // index od 1
    int fileIndex(const String& name);        // 0 = nie ma
    bool removeFile(const String& name);
    size_t freeSpace();
    fs::FS& fs();
    // Zamienia dowolna nazwe na bezpieczna nazwe pliku .sid ("" = niedozwolona).
    static String sanitizeName(const String& name);
    static const size_t MAX_FILE_SIZE = 0x10000 + 0x7C + 2;

    // --- odtwarzanie ---
    String play(const String& name, int song = 0);   // song 0 = domyslny z pliku
    String playIndex(int index, int song = 0);
    String nextFile();
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
    String loadAndStart(const String& name, int song);
    void lock();
    void unlock();
    static String path(const String& name) { return "/" + name; }

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
};
