/**
 * @file PlayerWebSkins.h
 * @brief Skins (looks) of the web panel. Each skin is a separate file with an HTML page.
 *
 * A new skin:
 *   1. copy e.g. PlayerWebPageC64.h as PlayerWebPageMine.h and rename the array
 *      to PLAYER_WEB_PAGE_MINE (the page uses the same /api/...),
 *   2. add a value to enum WebSkin (before Count),
 *   3. add a line to the WEB_SKINS table in PlayerWebSkins.cpp.
 *
 * Choosing a skin: /api/skin?s=<id> (the page has a "Wyglad" list for it).
 */
#pragma once
#include <Arduino.h>

/** @brief Identifier of a web panel skin; the value is stored in WebSettings. */
enum class WebSkin : uint8_t {
    Modern = 0,     ///< Default page (PlayerWebPage.h).
    C64,            ///< Commodore 64 screen (PlayerWebPageC64.h).
    Count           ///< Always last - number of skins.
};

/** @brief Description of one skin. */
struct WebSkinInfo {
    const char* id;       ///< Short name used in the API, e.g. "c64".
    const char* name;     ///< Name shown in the selection list (Polish; pages translate it by id, key skin_<id>).
    const char* page;     ///< HTML page (PROGMEM).
};

/// Table of skins, in the same order as enum WebSkin (PlayerWebSkins.cpp).
extern const WebSkinInfo WEB_SKINS[];
/// Number of skins.
constexpr uint8_t WEB_SKIN_COUNT = uint8_t(WebSkin::Count);
