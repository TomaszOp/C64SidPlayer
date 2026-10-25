// PlayerMemory - ustawienia odtwarzacza zapamietywane w NVS (mechanizm NvsSettings).
// Lista ustawien jest w PlayerSettings.def. Uzycie:
//
//   PlayerMemory mem;
//   mem.load();                 // w setup(): odczyt, brak zapisu = wartosci domyslne
//   mem.apply(player);          // glosnosc, model SID, zrodlo i foldery -> odtwarzacz
//   mem.capture(player);        // biezacy stan odtwarzacza -> ustawienia
//   mem.save();                 // zapisuje tylko to, co sie zmienilo
//   mem.setDefault(true);       // wartosci domyslne i wyczyszczenie NVS
#pragma once
#include <NvsSettings.h>
#include <SidSource.h>

#define NVS_CLASS     PlayerSettings
#define NVS_DEF       "PlayerSettings.def"
#define NVS_NAMESPACE "c64sid"
#define NVS_VERSION   1
#include <NvsSettingsClass.h>

class Player;

class PlayerMemory : public PlayerSettings {
public:
    // Pola zapamietane dla jednego zrodla (nullptr = brak pol dla tego zrodla).
    struct SourceMemory { String* dir; String* file; uint8_t* song; };
    SourceMemory forSource(SourceType type);

    void apply(Player& player);
    void capture(Player& player);
    // Ostatni plik i podutwor aktywnego zrodla - do grania po wlaczeniu zasilania.
    String lastFile(Player& player);
    uint8_t lastSong(Player& player);
};
