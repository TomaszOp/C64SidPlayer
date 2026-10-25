// ESP32 C64 SID Player - przyklad uzycia biblioteki Player.
//
//   lib/SidEngine      emulacja C64 (CPU 6510) i ukladu SID (wlasny silnik lub reSID)
//   lib/Player         biblioteka odtwarzacza: audio I2S, pliki z flash, sterowanie
//   lib/PlayerMemory   ustawienia zapamietywane w NVS (glosnosc, model SID, ostatni plik...)
//   lib/PlayerWeb      przyklad: panel WWW (telefon / PC)
//   lib/PlayerConsole  przyklad: komendy przez port szeregowy (COM)
//
// Ponizej mozna wylaczyc panel WWW albo konsole - odtwarzacz dziala bez nich.
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "config.h"
#include <Player.h>
#include <PlayerMemory.h>
#include <PlayerWeb.h>
#include <PlayerConsole.h>
#include <DualSerial.h>

#define USE_WEB      1
#define USE_CONSOLE  1
#define SAVE_EVERY_MS 5000   // co ile sprawdzac, czy ustawienia sie zmienily (oszczedza flash)

static DualSerial Console;
static Player player;
static PlayerMemory memory;
#if USE_WEB
static PlayerWeb web(player);
#endif
#if USE_CONSOLE
static PlayerConsole console(player, Console);
#endif

static void connectWiFi() {
    WiFi.setHostname(HOSTNAME);
    bool connected = false;
    if (strlen(WIFI_SSID)) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        Console.printf("Lacze z WiFi %s", WIFI_SSID);
        for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) { delay(250); Console.print('.'); }
        Console.println();
        connected = WiFi.status() == WL_CONNECTED;
    }
    if (connected) {
        WiFi.setSleep(false);   // bez oszczedzania energii - plynniejsze WWW przy grajacej muzyce
        Console.printf("Polaczono, otworz http://%s/ lub http://%s.local/\n",
                       WiFi.localIP().toString().c_str(), HOSTNAME);
    } else {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASSWORD);
        Console.printf("Punkt dostepowy %s (haslo %s), otworz http://%s/\n",
                       AP_SSID, AP_PASSWORD, WiFi.softAPIP().toString().c_str());
    }
    if (MDNS.begin(HOSTNAME)) MDNS.addService("http", "tcp", 80);
}

void setup() {
    Console.begin(115200);
    delay(200);
    Console.println("\n**** ESP32 C64 SID PLAYER ****");

    PlayerConfig cfg;
    cfg.sampleRate = SAMPLE_RATE;
    cfg.pinBck = I2S_PIN_BCK;
    cfg.pinWs = I2S_PIN_WS;
    cfg.pinData = I2S_PIN_DATA;
    cfg.volume = DEFAULT_VOLUME;
    if (!player.begin(cfg)) Console.println("Uwaga: problem z wyjsciem audio albo pamiecia plikow (LittleFS)");
    Console.printf("Silnik SID: %s, audio: %s\n", player.status().engine, player.audioError());

    // zapamietane ustawienia (pierwsze uruchomienie: wartosci z PlayerSettings.def)
    Console.println(memory.load() ? "Wczytano zapamietane ustawienia" : "Ustawienia domyslne");
    memory.apply(player);

#if USE_WEB
    connectWiFi();
    web.start();
#endif

    // po wlaczeniu zagraj ostatnio grany plik, a gdy go nie ma - pierwszy z pamieci
    if (memory.autoPlay && player.fileCount() > 0) {
        String e = "brak pliku";
        if (memory.lastFile.length() && player.fileIndex(memory.lastFile))
            e = player.play(memory.lastFile, memory.lastSong);
        if (e.length()) e = player.playIndex(1);
        if (e.length()) Console.println("blad: " + e);
    }
#if USE_CONSOLE
    console.setMemory(&memory);
    console.start();
    console.printStatus();
#endif
}

void loop() {
#if USE_WEB
    web.handle();
#endif
#if USE_CONSOLE
    console.handle();
#endif
    // zmiany z WWW i konsoli zapisuj co kilka sekund, i tylko gdy cos sie zmienilo
    static uint32_t lastSave = 0;
    if (millis() - lastSave > SAVE_EVERY_MS) {
        lastSave = millis();
        memory.capture(player);
        memory.save();
    }
    delay(2);
}
