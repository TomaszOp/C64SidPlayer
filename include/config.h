// Konfiguracja odtwarzacza SID - zmien wedlug swojego sprzetu i sieci.
#pragma once

// --- WiFi ---
// Jesli WIFI_SSID jest pusty albo polaczenie sie nie uda, ESP32 tworzy
// wlasny punkt dostepowy AP_SSID (haslo AP_PASSWORD), adres 192.168.4.1.
// Nazwa sieci i haslo sa w include/secrets.h (plik nie trafia do gita).
// Na nowym komputerze skopiuj secrets.example.h jako secrets.h i wpisz swoje dane.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif
#define AP_SSID        "ESP32-SID"
#define AP_PASSWORD    "commodore64"
#define HOSTNAME       "c64sid"          // http://c64sid.local

// --- Audio ---
#define SAMPLE_RATE    44100

// Zewnetrzny DAC I2S (PCM5102A, MAX98357A, UDA1334A...)
// Domyslne piny dla ESP32; srodowisko ESP32-S3 w platformio.ini ustawia wlasne.
#ifndef I2S_PIN_BCK
#define I2S_PIN_BCK    26
#endif
#ifndef I2S_PIN_WS
#define I2S_PIN_WS     25
#endif
#ifndef I2S_PIN_DATA
#define I2S_PIN_DATA   22
#endif
// Wbudowany 8-bitowy DAC (build flag -DSID_OUTPUT_INTERNAL_DAC) wychodzi na GPIO25 (i GPIO26).
// Dostepny tylko w klasycznym ESP32 - ESP32-S3 nie ma DAC, wymaga zewnetrznego DAC I2S.

// --- Karta SD (modul SPI) ---
// Wlaczana flaga -DPLAYER_SOURCE_SD=1 w platformio.ini. Zasilanie modulu: 3,3 V
// (albo 5 V, jesli modul ma wlasny stabilizator). Domyslne piny:
//   ESP32-S3: CS 10, MOSI 11, SCK 12, MISO 13      ESP32: CS 5, MOSI 23, SCK 18, MISO 19
#if CONFIG_IDF_TARGET_ESP32S3
#define SD_DEF_CS 10
#define SD_DEF_MOSI 11
#define SD_DEF_SCK 12
#define SD_DEF_MISO 13
#else
#define SD_DEF_CS 5
#define SD_DEF_MOSI 23
#define SD_DEF_SCK 18
#define SD_DEF_MISO 19
#endif
#ifndef SD_PIN_CS
#define SD_PIN_CS      SD_DEF_CS
#endif
#ifndef SD_PIN_MOSI
#define SD_PIN_MOSI    SD_DEF_MOSI
#endif
#ifndef SD_PIN_SCK
#define SD_PIN_SCK     SD_DEF_SCK
#endif
#ifndef SD_PIN_MISO
#define SD_PIN_MISO    SD_DEF_MISO
#endif
#define SD_SPI_FREQ    20000000          // obniz do 4000000, jesli karta nie dziala na dlugich kablach

// --- Prawdziwy uklad SID (silnik -DSID_ENGINE=2 w platformio.ini) ---
// Schemat podlaczenia: docs/REAL_SID.md
// Model ukladu w podstawce: 0 = MOS 6581 (VDD 12 V, kondensatory 470 pF),
//                           1 = MOS 8580 (VDD 9 V, kondensatory 22 nF)
#define REAL_SID_MODEL      1
// Piny (puste = domyslne z lib/SidEngine/real_sid_chip.h):
//   ESP32-S3: DATA 15, SHIFT 16, LATCH 17, CS 18, PHI2 7, RES 8
//   ESP32:    DATA 13, SHIFT 14, LATCH 27, CS 21, PHI2 32, RES 33
// #define REAL_SID_PIN_DATA   15   // 74HCT595 DS (pierwszy rejestr: D0-D7)
// #define REAL_SID_PIN_SHIFT  16   // 74HCT595 SHCP (oba rejestry)
// #define REAL_SID_PIN_LATCH  17   // 74HCT595 STCP (oba rejestry)
// #define REAL_SID_PIN_CS     18   // SID /CS
// #define REAL_SID_PIN_PHI2   7    // SID PHI2 (zegar ~1 MHz)
// #define REAL_SID_PIN_RES    8    // SID /RES
// 1 = emulacja reSID gra rownolegle przez DAC I2S (porownanie), 0 = cisza na I2S
#define REAL_SID_EMU_AUDIO  0

#define DEFAULT_VOLUME 56                // 0..100 (80 sciszone o 30%)
