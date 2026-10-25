// PlayerMemory - ustawienia odtwarzacza zapamietywane w NVS (mechanizm NvsSettings).
// Lista ustawien jest w PlayerSettings.def. Uzycie:
//
//   PlayerMemory mem;
//   mem.load();                 // w setup(): odczyt, brak zapisu = wartosci domyslne
//   mem.apply(player);          // glosnosc, wyciszenie, model SID -> odtwarzacz
//   mem.capture(player);        // biezacy stan odtwarzacza -> ustawienia
//   mem.save();                 // zapisuje tylko to, co sie zmienilo
//   mem.setDefault(true);       // wartosci domyslne i wyczyszczenie NVS
#pragma once
#include <NvsSettings.h>

#define NVS_CLASS     PlayerSettings
#define NVS_DEF       "PlayerSettings.def"
#define NVS_NAMESPACE "c64sid"
#define NVS_VERSION   1
#include <NvsSettingsClass.h>

class Player;

class PlayerMemory : public PlayerSettings {
public:
    void apply(Player& player) const;
    void capture(Player& player);
};
