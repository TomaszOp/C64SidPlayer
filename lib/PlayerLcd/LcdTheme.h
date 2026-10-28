/**
 * @file LcdTheme.h
 * @brief Colour themes of the LCD (RGB565): Modern and Commodore 64 (like the web panel skins).
 */
#pragma once
#include <stdint.h>

/// Converts 8-bit RGB to RGB565.
#define LCD_RGB(r, g, b) uint16_t((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

/** @brief Colours of one theme. */
struct LcdTheme {
    uint16_t bg;        ///< Background.
    uint16_t text;      ///< Normal text.
    uint16_t dim;       ///< Secondary text (author, details).
    uint16_t accent;    ///< Focus frame, volume bar, highlights.
    uint16_t editBg;    ///< Background of the field being edited.
    uint16_t selBg;     ///< Background of the highlighted list row.
    uint16_t selText;   ///< Text of the highlighted list row.
    uint16_t folder;    ///< Folder names.
    uint16_t playing;   ///< Marker and name of the file that is playing.
    uint16_t barBg;     ///< Empty part of the volume bar.
    uint16_t error;     ///< Messages / errors.
};

/** @brief Available themes (value stored in LcdSettings::theme). */
enum class LcdThemeId : uint8_t { Modern = 0, C64 = 1, Count };

/** @brief Theme table, indexed by LcdThemeId. */
static const LcdTheme LCD_THEMES[] = {
    // Modern: dark background, cyan accent
    {LCD_RGB(0, 0, 0), LCD_RGB(235, 235, 235), LCD_RGB(140, 140, 150), LCD_RGB(0, 190, 230),
     LCD_RGB(0, 50, 70), LCD_RGB(0, 120, 160), LCD_RGB(255, 255, 255), LCD_RGB(255, 200, 80),
     LCD_RGB(120, 230, 120), LCD_RGB(50, 50, 60), LCD_RGB(255, 110, 90)},
    // Commodore 64: blue screen, light blue text (Pepto palette)
    {LCD_RGB(0x40, 0x31, 0x8D), LCD_RGB(0x78, 0x69, 0xC4), LCD_RGB(0x9A, 0x8F, 0xD8), LCD_RGB(0xFF, 0xFF, 0xFF),
     LCD_RGB(0x2A, 0x20, 0x60), LCD_RGB(0x78, 0x69, 0xC4), LCD_RGB(0x40, 0x31, 0x8D), LCD_RGB(0xED, 0xF1, 0x71),
     LCD_RGB(0xA9, 0xFF, 0x9F), LCD_RGB(0x2A, 0x20, 0x60), LCD_RGB(0xFF, 0x77, 0x77)},
};
