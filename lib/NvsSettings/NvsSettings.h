// NvsSettings - wspolny mechanizm ustawien zapamietywanych w pamieci NVS ESP32
// (biblioteka Preferences). Odporne na zanik zasilania, nie kasowane przez
// "Upload Filesystem Image". Kazdy modul ma wlasna liste ustawien (plik .def)
// i wlasna przestrzen NVS, wiec moduly sa od siebie niezalezne.
//
// Nowy zestaw ustawien - plik naglowkowy, np. MojeUstawienia.h:
//
//   #pragma once
//   #define NVS_CLASS     MojeUstawienia        // nazwa klasy
//   #define NVS_DEF       "MojeUstawienia.def"  // lista ustawien
//   #define NVS_NAMESPACE "moje"                // przestrzen NVS (max 15 znakow)
//   #define NVS_VERSION   1                     // zwieksz po zmianie typu/znaczenia pola
//   #include <NvsSettingsClass.h>
//
// a w MojeUstawienia.def jedna linia na ustawienie:
//
//   SETTING(typ, nazwa, "klucz", wartosc_domyslna)
//
// Typy: bool, uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, float, String.
// Klucz: unikalny w przestrzeni, max 15 znakow. Uzycie:
//
//   MojeUstawienia s;
//   s.load();           // brak zapisu albo inna wersja = wartosci domyslne
//   s.jasnosc = 40;     // zwykle pola
//   s.save();           // zapisuje tylko wartosci rozne od tych w NVS
//   s.setDefault(true); // wartosci domyslne i wyczyszczenie NVS
#pragma once
#include <Arduino.h>

class NvsSettings {
public:
    NvsSettings(const char* nvsNamespace, uint16_t version) : ns_(nvsNamespace), version_(version) {}
    virtual ~NvsSettings() {}

    bool load();                         // false = brak zapisu albo inna wersja (zostaja domyslne)
    bool save();                         // false = blad NVS
    void setDefault(bool eraseNvs = false);   // bez kasowania: save() zapisze wartosci domyslne
    bool changed();                      // czy sa wartosci rozne od zapisanych
    void print(Print& out);              // wypisz "nazwa = wartosc" dla kazdego pola
    const char* nvsNamespace() const { return ns_; }

    // Odwiedzanie pol - generowane przez NvsSettingsClass.h z pliku .def.
    struct Visitor {
        virtual ~Visitor() {}
#define NVS_VISIT_TYPE(T) virtual void field(const char* key, const char* name, T& value, const T& def) = 0;
        NVS_VISIT_TYPE(bool) NVS_VISIT_TYPE(uint8_t) NVS_VISIT_TYPE(int8_t)
        NVS_VISIT_TYPE(uint16_t) NVS_VISIT_TYPE(int16_t) NVS_VISIT_TYPE(uint32_t)
        NVS_VISIT_TYPE(int32_t) NVS_VISIT_TYPE(float) NVS_VISIT_TYPE(String)
#undef NVS_VISIT_TYPE
    };

protected:
    virtual void visit(Visitor& v) = 0;

private:
    const char* ns_;
    uint16_t version_;
};
