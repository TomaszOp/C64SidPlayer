// Skorki (wyglady) panelu WWW. Kazda skorka to osobny plik ze strona HTML.
//
// Nowa skorka:
//   1. skopiuj np. PlayerWebPageC64.h jako PlayerWebPageMoja.h i zmien nazwe
//      tablicy na PLAYER_WEB_PAGE_MOJA (strona korzysta z tego samego /api/...),
//   2. dopisz wartosc do enum WebSkin (przed Count),
//   3. dopisz linie do tablicy WEB_SKINS w PlayerWebSkins.cpp.
// Wybor skorki: /api/skin?s=<id> (strona ma do tego liste "Wyglad").
#pragma once
#include <Arduino.h>

enum class WebSkin : uint8_t {
    Modern = 0,     // domyslna strona (PlayerWebPage.h)
    C64,            // ekran Commodore 64 (PlayerWebPageC64.h)
    Count           // zawsze ostatnie - liczba skorek
};

struct WebSkinInfo {
    const char* id;       // krotka nazwa w API, np. "c64"
    const char* name;     // nazwa na liscie wyboru
    const char* page;     // strona HTML (PROGMEM)
};

extern const WebSkinInfo WEB_SKINS[];
constexpr uint8_t WEB_SKIN_COUNT = uint8_t(WebSkin::Count);
