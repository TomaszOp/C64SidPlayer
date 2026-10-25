/**
 * @file PlayerWebLangs.cpp
 * @brief Table of the available web panel languages.
 */
#include "PlayerWebLangs.h"

// Same order as in enum WebLang.
const WebLangInfo WEB_LANGS[] = {
    {"pl", "Polski"},
    {"en", "English"},
};

static_assert(sizeof(WEB_LANGS) / sizeof(WEB_LANGS[0]) == WEB_LANG_COUNT,
              "WEB_LANGS must have as many entries as enum WebLang has values");
