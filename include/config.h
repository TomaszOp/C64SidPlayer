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

#define DEFAULT_VOLUME 56                // 0..100 (80 sciszone o 30%)
