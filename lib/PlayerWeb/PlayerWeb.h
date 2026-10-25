/**
 * @file PlayerWeb.h
 * @brief PlayerWeb - web panel for the Player library (an example of its use).
 *
 * The page works on phones and computers: playback control, volume, mute,
 * uploading and deleting .sid files. WiFi must be connected beforehand
 * (WiFi.begin / WiFi.softAP).
 *
 * @code
 *   PlayerWeb web(player);
 *   web.start();          // in setup()
 *   web.handle();         // in loop()
 * @endcode
 *
 * API (GET, JSON responses): /api/status, /api/list, /api/play?file=X|index=N[&song=S],
 * /api/pause, /api/resume, /api/stop, /api/song?n=N|d=+-1, /api/file?d=+-1,
 * /api/volume?v=0..100, /api/mute[?on=0|1], /api/model?m=-1|0|1, /api/delete?file=X,
 * /api/skins (list of skins), /api/skin?s=<id|number> (change the page look),
 * /api/langs (list of languages), /api/lang?l=<id|number> (change the page language),
 * /api/sources, /api/source?s=flash|sd|deepsid, /api/cd?dir=X|..|/, /api/list?offset=N&count=M
 * (listing of the current folder in chunks). Files: a name in the current folder or a full path.
 * Plus POST /upload (multipart, field "file").
 */
#pragma once
#include <WebServer.h>
#include "Player.h"
#include "PlayerWebSkins.h"
#include "PlayerWebLangs.h"
#include "WebSettings.h"

/** @brief HTTP server with the player web page and its JSON API. */
class PlayerWeb {
public:
    /**
     * @brief Creates the panel (the server is not started yet).
     * @param player Player to control (must outlive the panel).
     * @param port   HTTP port.
     */
    explicit PlayerWeb(Player& player, uint16_t port = 80);

    /** @brief Starts the HTTP server; on first start registers routes and loads the panel settings. */
    void start();
    /** @brief Stops the HTTP server. */
    void stop();
    /** @brief Handles pending HTTP requests - call often from loop(). */
    void handle();
    /** @brief @return true if the server is running. */
    bool running() const { return running_; }

    /** @brief @return Current page look (skins in PlayerWebSkins.h), remembered in NVS. */
    WebSkin skin() const;
    /**
     * @brief Changes the page look and saves it to NVS.
     * @param skin New skin (invalid values fall back to Modern).
     */
    void setSkin(WebSkin skin);

    /** @brief @return Current page language (languages in PlayerWebLangs.h), remembered in NVS. */
    WebLang lang() const;
    /**
     * @brief Changes the page language and saves it to NVS.
     * @param lang New language (invalid values fall back to Polish).
     */
    void setLang(WebLang lang);

    /** @brief @return The panel's own settings (WebSettings.def), loaded in start(). */
    WebSettings& settings() { return settings_; }

private:
    /** @brief Registers the page, all /api/... handlers and the upload handler. */
    void setupRoutes();
    /**
     * @brief Sends a command result: 200 {"ok":true} or 400 {"error":...}.
     * @param error Error description, "" = success.
     */
    void sendResult(const String& error);
    /** @brief @return Player status as JSON (/api/status). */
    String statusJson();
    /**
     * @brief Builds a chunk of the current folder listing as JSON (/api/list).
     * @param offset Index of the first entry; 0 or less also re-reads the folder.
     * @param count  Maximum number of entries in the chunk.
     * @return JSON object with folder info and the "items" array.
     */
    String listJson(int offset, int count);
    /** @brief @return Compiled-in sources and their state as JSON (/api/sources). */
    String sourcesJson();
    /** @brief @return Available skins and the current one as JSON (/api/skins). */
    String skinsJson();
    /** @brief @return Available languages and the current one as JSON (/api/langs). */
    String langsJson();
    /**
     * @brief Picks the panel's own message in the current page language.
     * @param pl Polish text.
     * @param en English text.
     * @return @p en when the page is in English, otherwise @p pl.
     */
    const char* tr(const char* pl, const char* en) const;
    /** @brief Receives upload chunks and writes them to the current folder of the active source (if allowed). */
    void handleUploadData();
    /** @brief Sends the upload result once all data was received. */
    void handleUploadDone();

    Player& player_;         ///< Controlled player.
    WebServer server_;       ///< HTTP server.
    bool running_;           ///< Server is running.
    bool routesReady_;       ///< Routes were registered.
    String uploadPath_;      ///< Destination path of the current upload.
    String uploadError_;     ///< Error of the current upload ("" = none).
    File uploadFile_;        ///< File being written by the upload.
    WebSettings settings_;   ///< Panel settings stored in NVS.
};
