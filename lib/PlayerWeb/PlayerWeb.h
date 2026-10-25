// PlayerWeb - panel WWW dla biblioteki Player (przyklad jej uzycia).
//
// Strona dziala na telefonie i komputerze: sterowanie odtwarzaniem,
// glosnosc, wyciszenie, wgrywanie i usuwanie plikow .sid.
// WiFi trzeba polaczyc wczesniej (WiFi.begin / WiFi.softAP).
//
//   PlayerWeb web(player);
//   web.start();          // w setup()
//   web.handle();         // w loop()
//
// API (GET, odpowiedzi JSON): /api/status, /api/list, /api/play?file=X|index=N[&song=S],
// /api/pause, /api/resume, /api/stop, /api/song?n=N|d=+-1, /api/file?d=+-1,
// /api/volume?v=0..100, /api/mute[?on=0|1], /api/model?m=-1|0|1, /api/delete?file=X,
// /api/skins (lista skorek), /api/skin?s=<id|numer> (zmiana wygladu strony)
// oraz POST /upload (multipart, pole "file").
#pragma once
#include <WebServer.h>
#include "Player.h"
#include "PlayerWebSkins.h"
#include "WebSettings.h"

class PlayerWeb {
public:
    explicit PlayerWeb(Player& player, uint16_t port = 80);

    void start();
    void stop();
    void handle();                 // wywoluj czesto w loop()
    bool running() const { return running_; }

    // wyglad strony (skorki w PlayerWebSkins.h), zapamietywany w NVS
    WebSkin skin() const;
    void setSkin(WebSkin skin);

    // wlasne ustawienia panelu (WebSettings.def) - wczytywane w start()
    WebSettings& settings() { return settings_; }

private:
    void setupRoutes();
    void sendResult(const String& error);
    String statusJson();
    String listJson();
    String skinsJson();
    void handleUploadData();
    void handleUploadDone();

    Player& player_;
    WebServer server_;
    bool running_;
    bool routesReady_;
    String uploadName_;
    File uploadFile_;
    WebSettings settings_;
};
