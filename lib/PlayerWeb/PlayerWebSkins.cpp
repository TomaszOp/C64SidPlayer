/**
 * @file PlayerWebSkins.cpp
 * @brief Table of the available web panel skins.
 */
#include "PlayerWebSkins.h"
#include "PlayerWebPage.h"
#include "PlayerWebPageC64.h"

// Same order as in enum WebSkin.
const WebSkinInfo WEB_SKINS[] = {
    {"modern", "Nowoczesny",   PLAYER_WEB_PAGE},
    {"c64",    "Commodore 64", PLAYER_WEB_PAGE_C64},
};

static_assert(sizeof(WEB_SKINS) / sizeof(WEB_SKINS[0]) == WEB_SKIN_COUNT,
              "WEB_SKINS musi miec tyle pozycji, ile jest wartosci w enum WebSkin");
