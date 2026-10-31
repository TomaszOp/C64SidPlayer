/**
 * @file main.cpp
 * @brief ESP32 C64 SID Player - example of using the Player library.
 *
 * @code
 *   lib/SidEngine      C64 emulation (6510 CPU) and the SID chip (own engine, reSID or a real chip)
 *   lib/Player         player library: I2S audio, files from flash / SD / DeepSID, control
 *   lib/PlayerMemory   settings remembered in NVS (volume, SID model, last file...)
 *   lib/PlayerWeb      example: web panel (phone / PC)
 *   lib/PlayerConsole  example: commands over the serial port (COM)
 *   lib/PlayerLcd      optional: ILI9341 LCD + encoder + buttons (-DUSE_LCD=1, docs/LCD.md)
 * @endcode
 * The web panel or the console can be disabled below - the player works without them.
 */
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "config.h"
#include <Player.h>
#include <PlayerMemory.h>
#include <PlayerWeb.h>
#include <PlayerConsole.h>
#include <DualSerial.h>
#if USE_LCD
#include <PlayerLcd.h>
#endif

#define USE_WEB      1     ///< 1 = start WiFi and the web panel.
#define USE_CONSOLE  1     ///< 1 = serial console commands.
#define SAVE_EVERY_MS 5000   ///< How often to check whether the settings changed, in ms (saves flash).

static DualSerial Console;     ///< Console on USB CDC and UART.
static Player player;          ///< The SID player.
static PlayerMemory memory;    ///< Settings remembered in NVS.
#if USE_WEB
static PlayerWeb web(player);  ///< Web panel.
#endif
#if USE_CONSOLE
static PlayerConsole console(player, Console);   ///< Serial command console.
#endif
#if USE_LCD
static PlayerLcd lcd(player);  ///< LCD with the encoder and buttons.

/** @brief Starts the LCD with the pins from config.h. */
static void startLcd() {
    LcdConfig c;
    c.pinBacklight = LCD_PIN_BL;
    c.rotation = LCD_ROTATION;
    c.input.pinA = LCD_PIN_ENC_A;
    c.input.pinB = LCD_PIN_ENC_B;
    c.input.pinPush = LCD_PIN_PUSH;
    c.input.pinBack = LCD_BUTTONS ? LCD_PIN_BACK : -1;
    c.input.pinSelect = LCD_BUTTONS ? LCD_PIN_SELECT : -1;
    c.input.countsPerDetent = LCD_ENC_COUNTS;
    c.input.reverse = LCD_ENC_REVERSE;
    lcd.begin(c);
}
#endif

/**
 * @brief Connects to WIFI_SSID (10 s timeout) or, failing that, starts the AP_SSID access point;
 *        then starts mDNS (HOSTNAME.local).
 */
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

/**
 * @brief Arduino setup: starts the console, the player, restores the settings, WiFi and the web panel,
 *        and auto-plays the last file (or the first one in the folder).
 */
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
    cfg.sdCs = SD_PIN_CS;
    cfg.sdSck = SD_PIN_SCK;
    cfg.sdMiso = SD_PIN_MISO;
    cfg.sdMosi = SD_PIN_MOSI;
    cfg.sdFreq = SD_SPI_FREQ;
    cfg.sdFatDrive = SD_FAT_DRIVE;
    if (!player.begin(cfg)) Console.println("Uwaga: problem z wyjsciem audio albo pamiecia plikow (LittleFS)");
    Console.printf("Silnik SID: %s, audio: %s\n", player.status().engine, player.audioError());

    // zapamietane ustawienia (pierwsze uruchomienie: wartosci z PlayerSettings.def)
    Console.println(memory.load() ? "Wczytano zapamietane ustawienia" : "Ustawienia domyslne");
    memory.apply(player);
    Console.printf("Zrodlo plikow: %s, folder %s%s\n", player.sourceInfo().name, player.dir().c_str(),
                   player.sourceReady() ? "" : (" (" + player.sourceError() + ")").c_str());

#if USE_WEB
    connectWiFi();
    web.start();
#endif

    // po wlaczeniu zagraj ostatni plik aktywnego zrodla, a gdy go nie ma - pierwszy z folderu
    if (memory.autoPlay && player.sourceReady()) {
        String last = memory.lastFile(player);
        String e = last.length() ? player.play(last, memory.lastSong(player)) : String("brak");
        if (e.length() && player.fileCount() > 0) e = player.playIndex(1);
        else if (e.length()) e = "";            // pusty folder - nic do grania
        if (e.length()) Console.println("blad: " + e);
    }
    // read the current folder now, so that the first page load does not wait for it (DeepSid: seconds)
    if (player.sourceReady()) player.files();
#if USE_LCD
    startLcd();
#endif
#if USE_CONSOLE
    console.setMemory(&memory);
    console.start();
    console.printStatus();
#endif
}

/**
 * @brief Arduino loop: serves the web panel, the console and the LCD and saves changed settings every SAVE_EVERY_MS.
 */
void loop() {
#if USE_WEB
    web.handle();
#endif
#if USE_CONSOLE
    console.handle();
#endif
#if USE_LCD
    lcd.handle();
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
