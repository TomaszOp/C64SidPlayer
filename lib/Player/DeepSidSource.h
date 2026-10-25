// DeepSidSource - pliki .sid z portalu DeepSID (https://deepsid.chordian.net),
// czyli kolekcja HVSC pobierana przez internet. Wlaczane flaga -DPLAYER_SOURCE_DEEPSID=1.
//
// Lista folderu pochodzi z publicznego REST API DeepSID (/api/v1/, bez klucza):
//   ?folder=X   podfoldery X (API zwraca wszystkie poziomy - zostawiamy tylko bezposrednie),
//   ?file=X/    pliki .sid w folderze X.
// Sam plik .sid jest pobierany jak na stronie: /music/_High Voltage SID Collection/X/plik.sid.
//
// Sciezki zrodla sa sciezkami HVSC, np. "/MUSICIANS/H/Hubbard_Rob/Commando.sid".
// Polaczenie HTTPS bez sprawdzania certyfikatu (publiczne pliki, brak hasel).
// Uwaga: folder /MUSICIANS ma tysiace podfolderow na nizszych poziomach, wiec jego
// lista wczytuje sie kilka-kilkanascie sekund.
#pragma once
#include "SidSource.h"

#if PLAYER_SOURCE_DEEPSID
class DeepSidSource : public SidSource {
public:
    bool begin(const PlayerConfig& cfg) override { return true; }
    bool ready() override;                          // WiFi polaczone i brak swiezego bledu sieci
    const char* error() const override { return error_.c_str(); }

    bool list(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) override;
    bool isDir(const String& path) override;
    String read(const String& path, uint8_t*& buf, size_t& len, size_t maxLen) override;

private:
    bool listFolders(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries);
    bool listFiles(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries);
    void netFailed(const String& why);

    String error_ = "brak polaczenia z internetem";
    bool netError_ = false;
    uint32_t netErrorAt_ = 0;
    String cachedDir_;                  // ostatnio wczytany folder i jego podfoldery (dla isDir)
    std::vector<String> cachedDirs_;
};
#endif
