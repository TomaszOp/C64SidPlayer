/**
 * @file PlayerWeb.cpp
 * @brief Implementation of the web panel: HTTP routes, JSON responses and uploads.
 */
#include "PlayerWeb.h"
#include <algorithm>

/**
 * @brief Converts a string to a quoted JSON string literal.
 *
 * Strings in SID files are ISO-8859-1; they are converted to UTF-8 with JSON
 * escaping (control characters become spaces).
 *
 * @param s Input string (ISO-8859-1).
 * @return Quoted, escaped UTF-8 JSON string.
 */
static String jsonStr(const String& s) {
    String out = "\"";
    for (size_t k = 0; k < s.length(); k++) {
        uint8_t c = uint8_t(s[k]);
        if (c == '"' || c == '\\') { out += '\\'; out += char(c); }
        else if (c < 0x20) out += ' ';
        else if (c < 0x80) out += char(c);
        else { out += char(0xC0 | (c >> 6)); out += char(0x80 | (c & 0x3F)); }
    }
    return out + "\"";
}

/**
 * @brief Returns the API name of a playback state.
 * @param s Playback state.
 * @return "playing", "paused" or "stopped".
 */
static const char* stateName(PlayerState s) {
    switch (s) {
    case PlayerState::Playing: return "playing";
    case PlayerState::Paused: return "paused";
    default: return "stopped";
    }
}

PlayerWeb::PlayerWeb(Player& player, uint16_t port)
    : player_(player), server_(port), running_(false), routesReady_(false) {}

void PlayerWeb::start() {
    if (running_) return;
    if (!routesReady_) { setupRoutes(); routesReady_ = true; settings_.load(); }
    server_.begin();
    running_ = true;
}

void PlayerWeb::stop() {
    if (!running_) return;
    server_.stop();
    running_ = false;
}

void PlayerWeb::handle() {
    if (running_) server_.handleClient();
}

WebSkin PlayerWeb::skin() const {
    return settings_.skin < WEB_SKIN_COUNT ? WebSkin(settings_.skin) : WebSkin::Modern;
}

void PlayerWeb::setSkin(WebSkin skin) {
    settings_.skin = uint8_t(skin) < WEB_SKIN_COUNT ? uint8_t(skin) : 0;
    settings_.save();
}

WebLang PlayerWeb::lang() const {
    return settings_.lang < WEB_LANG_COUNT ? WebLang(settings_.lang) : WebLang::Pl;
}

void PlayerWeb::setLang(WebLang lang) {
    settings_.lang = uint8_t(lang) < WEB_LANG_COUNT ? uint8_t(lang) : 0;
    settings_.save();
}

const char* PlayerWeb::tr(const char* pl, const char* en) const {
    return lang() == WebLang::En ? en : pl;
}

void PlayerWeb::sendResult(const String& error) {
    if (error.length()) server_.send(400, "application/json", "{\"error\":" + jsonStr(error) + "}");
    else server_.send(200, "application/json", "{\"ok\":true}");
}

String PlayerWeb::statusJson() {
    PlayerStatus s = player_.status();
    String j = "{\"state\":\"" + String(stateName(s.state)) + "\"";
    j += ",\"loaded\":" + String(s.loaded ? "true" : "false");
    j += ",\"file\":" + jsonStr(s.file);
    if (s.loaded) {
        j += ",\"name\":" + jsonStr(s.title);
        j += ",\"author\":" + jsonStr(s.author);
        j += ",\"released\":" + jsonStr(s.released);
        j += ",\"song\":" + String(s.song);
        j += ",\"songs\":" + String(s.songs);
        j += ",\"type\":\"" + String(s.rsid ? "RSID" : "PSID") + "\"";
        j += ",\"clock\":\"" + String(s.ntsc ? "NTSC" : "PAL") + "\"";
        j += ",\"model\":\"" + String(s.sidModel ? "8580" : "6581") + "\"";
        j += ",\"hz\":" + String(s.frameHz, 1);
        j += ",\"ms\":" + String(s.ms);
    }
    j += ",\"volume\":" + String(s.volume);
    j += ",\"muted\":" + String(s.muted ? "true" : "false");
    j += ",\"modelSetting\":" + String(s.modelSetting);
    j += ",\"engine\":" + jsonStr(s.engine);
    j += ",\"load\":" + String(s.cpuLoad);
    j += ",\"source\":" + jsonStr(player_.sourceInfo().id);
    j += ",\"dir\":" + jsonStr(s.dir);
    j += ",\"fileSource\":" + jsonStr(::sourceInfo(s.fileSource) ? ::sourceInfo(s.fileSource)->id : "");
    // zrodla bez wlasnej pamieci (internet) nie podaja wolnego miejsca
    if (player_.totalSpace()) j += ",\"freeFs\":" + String(double(player_.freeSpace()), 0);
    j += ",\"skin\":" + jsonStr(WEB_SKINS[uint8_t(skin())].id);
    j += ",\"lang\":" + jsonStr(WEB_LANGS[uint8_t(lang())].id);
    return j + "}";
}

String PlayerWeb::listJson(int offset, int count) {
    // first chunk: re-read local storage (uploads, SD card swap); a remote listing (DeepSid, several
    // HTTPS requests) is kept until the folder changes, unless it came back empty (network error)
    if (offset <= 0 && (player_.activeSource().canWrite() || player_.files().empty())) player_.refreshFiles();
    const std::vector<PlayerFile>& list = player_.files();
    bool ready = player_.sourceReady();
    String j = "{\"source\":" + jsonStr(player_.sourceInfo().id);
    j += ",\"dir\":" + jsonStr(player_.dir());
    j += ",\"ready\":" + String(ready ? "true" : "false");
    j += ",\"error\":" + jsonStr(ready ? String("") : player_.sourceError());
    j += ",\"upload\":" + String(player_.uploadAllowed() ? "true" : "false");
    j += ",\"del\":" + String(player_.deleteAllowed() ? "true" : "false");
    j += ",\"total\":" + String(unsigned(list.size()));
    j += ",\"truncated\":" + String(player_.filesTruncated() ? "true" : "false");
    j += ",\"offset\":" + String(offset) + ",\"items\":[";
    int end = std::min(int(list.size()), offset + count);
    for (int i = std::max(offset, 0); i < end; i++) {
        if (i > offset) j += ",";
        j += "{\"name\":" + jsonStr(list[i].name) + ",\"size\":" + String(unsigned(list[i].size));
        if (list[i].dir) j += ",\"dir\":true";
        j += "}";
    }
    return j + "]}";
}

String PlayerWeb::sourcesJson() {
    String j = "{\"current\":" + jsonStr(player_.sourceInfo().id) + ",\"sources\":[";
    for (uint8_t i = 0; i < PLAYER_SOURCE_COUNT; i++) {
        const SourceInfo& s = PLAYER_SOURCES[i];
        SidSource* impl = sourceImpl(s.type);
        if (i) j += ",";
        j += "{\"id\":" + jsonStr(s.id) + ",\"name\":" + jsonStr(s.name);
        j += ",\"upload\":" + String(s.upload ? "true" : "false");
        j += ",\"ready\":" + String(impl && impl->ready() ? "true" : "false") + "}";
    }
    return j + "]}";
}

String PlayerWeb::skinsJson() {
    String j = "{\"current\":" + jsonStr(WEB_SKINS[uint8_t(skin())].id) + ",\"skins\":[";
    for (uint8_t i = 0; i < WEB_SKIN_COUNT; i++) {
        if (i) j += ",";
        j += "{\"id\":" + jsonStr(WEB_SKINS[i].id) + ",\"name\":" + jsonStr(WEB_SKINS[i].name) + "}";
    }
    return j + "]}";
}

String PlayerWeb::langsJson() {
    String j = "{\"current\":" + jsonStr(WEB_LANGS[uint8_t(lang())].id) + ",\"langs\":[";
    for (uint8_t i = 0; i < WEB_LANG_COUNT; i++) {
        if (i) j += ",";
        j += "{\"id\":" + jsonStr(WEB_LANGS[i].id) + ",\"name\":" + jsonStr(WEB_LANGS[i].name) + "}";
    }
    return j + "]}";
}

void PlayerWeb::setupRoutes() {
    server_.on("/", HTTP_GET, [this] {
        // the page reads the language from this cookie before it is first drawn
        server_.sendHeader("Set-Cookie", String("lang=") + WEB_LANGS[uint8_t(lang())].id + "; Path=/; Max-Age=31536000");
        server_.send_P(200, "text/html", WEB_SKINS[uint8_t(skin())].page);
    });
    server_.on("/api/skins", HTTP_GET, [this] { server_.send(200, "application/json", skinsJson()); });
    server_.on("/api/skin", HTTP_GET, [this] {
        // ?s=c64 albo ?s=1
        String s = server_.arg("s");
        for (uint8_t i = 0; i < WEB_SKIN_COUNT; i++)
            if (s == WEB_SKINS[i].id || s == String(i)) { setSkin(WebSkin(i)); sendResult(""); return; }
        sendResult(tr("nieznana skorka: ", "unknown skin: ") + s);
    });
    server_.on("/api/langs", HTTP_GET, [this] { server_.send(200, "application/json", langsJson()); });
    server_.on("/api/lang", HTTP_GET, [this] {
        // ?l=en or ?l=1
        String l = server_.arg("l");
        for (uint8_t i = 0; i < WEB_LANG_COUNT; i++)
            if (l == WEB_LANGS[i].id || l == String(i)) { setLang(WebLang(i)); sendResult(""); return; }
        sendResult(tr("nieznany jezyk: ", "unknown language: ") + l);
    });
    server_.on("/api/status", HTTP_GET, [this] { server_.send(200, "application/json", statusJson()); });
    server_.on("/api/list", HTTP_GET, [this] {
        int count = server_.hasArg("count") ? server_.arg("count").toInt() : 100;
        server_.send(200, "application/json", listJson(server_.arg("offset").toInt(), constrain(count, 1, 200)));
    });
    server_.on("/api/cd", HTTP_GET, [this] { sendResult(player_.cd(server_.arg("dir"))); });
    server_.on("/api/sources", HTTP_GET, [this] { server_.send(200, "application/json", sourcesJson()); });
    server_.on("/api/source", HTTP_GET, [this] {
        const SourceInfo* info = ::sourceInfo(server_.arg("s"));
        if (!info) { sendResult(tr("nieznane zrodlo: ", "unknown source: ") + server_.arg("s")); return; }
        sendResult(player_.setSource(info->type));
    });
    server_.on("/api/play", HTTP_GET, [this] {
        int song = server_.arg("song").toInt();
        if (server_.hasArg("index")) sendResult(player_.playIndex(server_.arg("index").toInt(), song));
        else sendResult(player_.play(server_.arg("file"), song));
    });
    server_.on("/api/pause", HTTP_GET, [this] { player_.togglePause(); sendResult(""); });
    server_.on("/api/resume", HTTP_GET, [this] { player_.resume(); sendResult(""); });
    server_.on("/api/stop", HTTP_GET, [this] { player_.stop(); sendResult(""); });
    server_.on("/api/song", HTTP_GET, [this] {
        if (server_.hasArg("n")) sendResult(player_.setSong(server_.arg("n").toInt()));
        else sendResult(server_.arg("d").toInt() < 0 ? player_.prevSong() : player_.nextSong());
    });
    server_.on("/api/file", HTTP_GET, [this] {
        sendResult(server_.arg("d").toInt() < 0 ? player_.prevFile() : player_.nextFile());
    });
    server_.on("/api/volume", HTTP_GET, [this] { player_.setVolume(server_.arg("v").toInt()); sendResult(""); });
    server_.on("/api/mute", HTTP_GET, [this] {
        if (server_.hasArg("on")) player_.setMute(server_.arg("on").toInt() != 0);
        else player_.toggleMute();
        sendResult("");
    });
    server_.on("/api/model", HTTP_GET, [this] { sendResult(player_.setModel(server_.arg("m").toInt())); });
    server_.on("/api/delete", HTTP_GET, [this] {
        sendResult(player_.removeFile(server_.arg("file")) ? String("") : String(tr("nie usunieto", "not deleted")));
    });
    server_.on("/upload", HTTP_POST, [this] { handleUploadDone(); }, [this] { handleUploadData(); });
    server_.onNotFound([this] { server_.send(404, "text/plain", "404"); });
}

void PlayerWeb::handleUploadData() {
    HTTPUpload& up = server_.upload();
    if (up.status == UPLOAD_FILE_START) {
        uploadError_ = player_.prepareUpload(up.filename, uploadPath_);
        if (!uploadError_.length()) {
            uploadFile_ = player_.openWrite(uploadPath_);
            if (!uploadFile_) uploadError_ = tr("nie mozna zapisac ", "cannot write ") + uploadPath_;
        }
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (uploadFile_ && uploadFile_.write(up.buf, up.currentSize) != up.currentSize && !uploadError_.length())
            uploadError_ = tr("brak miejsca", "no space left");
    } else if (up.status == UPLOAD_FILE_END) {
        if (uploadFile_) uploadFile_.close();
        if (!uploadError_.length() && up.totalSize > Player::MAX_FILE_SIZE) uploadError_ = tr("plik za duzy (max 64 KB)", "file too large (max 64 KB)");
        if (uploadError_.length() && uploadPath_.length()) player_.removePath(uploadPath_);
    } else if (up.status == UPLOAD_FILE_ABORTED) {
        if (uploadFile_) uploadFile_.close();
        if (uploadPath_.length()) player_.removePath(uploadPath_);
        uploadError_ = tr("przerwano wysylanie", "upload aborted");
    }
}

void PlayerWeb::handleUploadDone() {
    player_.refreshFiles();
    if (uploadError_.length() || !uploadPath_.length()) {
        sendResult(uploadError_.length() ? uploadError_ : String(tr("brak pliku", "no file")));
        return;
    }
    String e = player_.play(uploadPath_);
    if (e.length()) player_.removePath(uploadPath_);   // to nie byl poprawny plik SID
    sendResult(e);
}
