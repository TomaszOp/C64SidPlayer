/**
 * @file config.h
 * @brief SID player configuration - change it to match your hardware and network.
 */
#pragma once

// --- WiFi ---
// If WIFI_SSID is empty or the connection fails, the ESP32 creates
// its own access point AP_SSID (password AP_PASSWORD), address 192.168.4.1.
// The network name and password are in include/secrets.h (the file is not tracked by git).
// On a new computer copy secrets.example.h to secrets.h and enter your data.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif
#define AP_SSID        "ESP32-SID"       ///< Name of the fallback access point.
#define AP_PASSWORD    "commodore64"     ///< Password of the fallback access point.
#define HOSTNAME       "c64sid"          ///< mDNS / DHCP host name: http://c64sid.local

// --- Audio ---
#define SAMPLE_RATE    44100             ///< Audio sample rate in Hz.

// External I2S DAC (PCM5102A, MAX98357A, UDA1334A...)
// Default pins for the ESP32; the ESP32-S3 environment in platformio.ini sets its own.
#ifndef I2S_PIN_BCK
#define I2S_PIN_BCK    26                ///< I2S bit clock (BCK) pin.
#endif
#ifndef I2S_PIN_WS
#define I2S_PIN_WS     25                ///< I2S word select (WS / LRCK) pin.
#endif
#ifndef I2S_PIN_DATA
#define I2S_PIN_DATA   22                ///< I2S data out (DIN) pin.
#endif
// The built-in 8-bit DAC (build flag -DSID_OUTPUT_INTERNAL_DAC) outputs on GPIO25 (and GPIO26).
// Available only on the classic ESP32 - the ESP32-S3 has no DAC and needs an external I2S DAC.

// --- SD card (SPI module) ---
// Enabled with the -DPLAYER_SOURCE_SD=1 flag in platformio.ini. Module power: 3.3 V
// (or 5 V if the module has its own regulator). Default pins:
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
#define SD_PIN_CS      SD_DEF_CS         ///< SD card chip select pin.
#endif
#ifndef SD_PIN_MOSI
#define SD_PIN_MOSI    SD_DEF_MOSI       ///< SD card MOSI pin.
#endif
#ifndef SD_PIN_SCK
#define SD_PIN_SCK     SD_DEF_SCK        ///< SD card SCK pin.
#endif
#ifndef SD_PIN_MISO
#define SD_PIN_MISO    SD_DEF_MISO       ///< SD card MISO pin.
#endif
#define SD_SPI_FREQ    20000000          ///< SD SPI clock in Hz; lower it to 4000000 if the card fails on long wires.

// --- Real SID chip (engine -DSID_ENGINE=2 in platformio.ini) ---
// Wiring diagram: docs/REAL_SID.md
// Model of the chip in the socket: 0 = MOS 6581 (VDD 12 V, 470 pF capacitors),
//                                  1 = MOS 8580 (VDD 9 V, 22 nF capacitors)
#define REAL_SID_MODEL      1
// Pins (commented out = defaults from lib/SidEngine/real_sid_chip.h):
//   ESP32-S3: DATA 15, SHIFT 16, LATCH 17, CS 18, PHI2 7, RES 8
//   ESP32:    DATA 13, SHIFT 14, LATCH 27, CS 21, PHI2 32, RES 33
// #define REAL_SID_PIN_DATA   15   // 74HCT595 DS (first register: D0-D7)
// #define REAL_SID_PIN_SHIFT  16   // 74HCT595 SHCP (both registers)
// #define REAL_SID_PIN_LATCH  17   // 74HCT595 STCP (both registers)
// #define REAL_SID_PIN_CS     18   // SID /CS
// #define REAL_SID_PIN_PHI2   7    // SID PHI2 (clock ~1 MHz)
// #define REAL_SID_PIN_RES    8    // SID /RES
// 1 = the reSID emulation plays in parallel through the I2S DAC (comparison), 0 = silence on I2S
#define REAL_SID_EMU_AUDIO  0

// --- LCD ILI9341 + encoder + buttons (-DUSE_LCD=1, environment esp32s3-n16r8-lcd) ---
// Description and wiring: docs/LCD.md. The display pins (TFT_SCLK, TFT_MOSI, TFT_CS, TFT_DC,
// TFT_RST) are TFT_eSPI build flags in platformio.ini; the remaining pins are here.
#ifndef USE_LCD
#define USE_LCD 0
#endif
#ifndef LCD_BUTTONS
#define LCD_BUTTONS    1     ///< 1 = encoder + buttons Back / Select, 0 = encoder only (short push = Select, long = Back).
#endif
#define LCD_PIN_BL     2     ///< Backlight PWM (through a transistor), -1 = backlight wired to 3V3.
#define LCD_PIN_ENC_A  40    ///< Encoder A (CLK).
#define LCD_PIN_ENC_B  41    ///< Encoder B (DT).
#define LCD_PIN_PUSH   42    ///< Encoder push button (to GND).
#define LCD_PIN_BACK   1     ///< Button 1 "Back" (to GND), used when LCD_BUTTONS = 1.
#define LCD_PIN_SELECT 39    ///< Button 2 "Select" (to GND), used when LCD_BUTTONS = 1.
#define LCD_ROTATION   1     ///< 1 or 3 = landscape (3 = rotated by 180 degrees).
#define LCD_ENC_COUNTS 4     ///< Encoder counts per click: 4 for most encoders, 2 if one click moves by two.
#define LCD_ENC_REVERSE 0    ///< 1 = swap the turning direction.

#if USE_LCD
#if !CONFIG_IDF_TARGET_ESP32S3
#error "LCD (USE_LCD=1) jest przewidziany tylko dla ESP32-S3 (srodowisko esp32s3-n16r8-lcd)"
#endif
#if defined(TFT_CS) && (TFT_CS == SD_PIN_CS || TFT_SCLK == SD_PIN_SCK || TFT_MOSI == SD_PIN_MOSI)
#error "Piny LCD (TFT_*) koliduja z karta SD - LCD ma osobna magistrale SPI"
#endif
#if defined(TFT_CS) && (TFT_CS == I2S_PIN_BCK || TFT_CS == I2S_PIN_WS || TFT_DC == I2S_PIN_DATA)
#error "Piny LCD (TFT_*) koliduja z I2S"
#endif
#endif

#define DEFAULT_VOLUME 56                ///< Default volume 0..100 (80 lowered by 30%).
