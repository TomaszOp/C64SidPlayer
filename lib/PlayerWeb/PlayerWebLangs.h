/**
 * @file PlayerWebLangs.h
 * @brief Languages of the web panel. Each skin page carries its own translations
 *        (a dictionary in its JavaScript), the server only remembers the choice.
 *
 * A new language:
 *   1. add a value to enum WebLang (before Count),
 *   2. add a line to the WEB_LANGS table in PlayerWebLangs.cpp,
 *   3. add a dictionary with the same id to every skin page (PlayerWebPage*.h).
 *
 * Choosing a language: /api/lang?l=<id> (the page has a "Jezyk / Language" list for it).
 */
#pragma once
#include <Arduino.h>

/** @brief Identifier of a web panel language; the value is stored in WebSettings. */
enum class WebLang : uint8_t {
    Pl = 0,         ///< Polish (default).
    En,             ///< English.
    Count           ///< Always last - number of languages.
};

/** @brief Description of one language. */
struct WebLangInfo {
    const char* id;       ///< Short name used in the API and in the page dictionaries, e.g. "en".
    const char* name;     ///< Native name shown in the selection list.
};

/// Table of languages, in the same order as enum WebLang (PlayerWebLangs.cpp).
extern const WebLangInfo WEB_LANGS[];
/// Number of languages.
constexpr uint8_t WEB_LANG_COUNT = uint8_t(WebLang::Count);
