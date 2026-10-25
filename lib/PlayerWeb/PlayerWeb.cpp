#include "PlayerWeb.h"
#include "PlayerWebPage.h"

// Napisy w plikach SID sa w ISO-8859-1; zamiana na UTF-8 z escapowaniem JSON.
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
    if (!routesReady_) { setupRoutes(); routesReady_ = true; }
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
    j += ",\"freeFs\":" + String(player_.freeSpace());
    return j + "}";
}

String PlayerWeb::listJson() {
    String j = "[";
    bool first = true;
    for (const PlayerFile& f : player_.files()) {
        if (!first) j += ",";
        first = false;
        j += "{\"name\":" + jsonStr(f.name) + ",\"size\":" + String(f.size) + "}";
    }
    return j + "]";
}

void PlayerWeb::setupRoutes() {
    server_.on("/", HTTP_GET, [this] { server_.send_P(200, "text/html", PLAYER_WEB_PAGE); });
    server_.on("/api/status", HTTP_GET, [this] { server_.send(200, "application/json", statusJson()); });
    server_.on("/api/list", HTTP_GET, [this] { server_.send(200, "application/json", listJson()); });
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
        sendResult(player_.removeFile(server_.arg("file")) ? String("") : String("nie usunieto"));
    });
    server_.on("/upload", HTTP_POST, [this] { handleUploadDone(); }, [this] { handleUploadData(); });
    server_.onNotFound([this] { server_.send(404, "text/plain", "404"); });
}

void PlayerWeb::handleUploadData() {
    HTTPUpload& up = server_.upload();
    if (up.status == UPLOAD_FILE_START) {
        uploadName_ = Player::sanitizeName(up.filename);
        if (uploadName_.length()) {
            if (uploadName_ == player_.status().file) player_.stop();
            uploadFile_ = player_.fs().open("/" + uploadName_, "w");
        }
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (uploadFile_) uploadFile_.write(up.buf, up.currentSize);
    } else if (up.status == UPLOAD_FILE_END) {
        if (uploadFile_) uploadFile_.close();
        if (up.totalSize > Player::MAX_FILE_SIZE && uploadName_.length()) {
            player_.fs().remove("/" + uploadName_);
            uploadName_ = "";
        }
    } else if (up.status == UPLOAD_FILE_ABORTED) {
        if (uploadFile_) uploadFile_.close();
        if (uploadName_.length()) player_.fs().remove("/" + uploadName_);
        uploadName_ = "";
    }
}

void PlayerWeb::handleUploadDone() {
    if (!uploadName_.length()) { sendResult("dozwolone sa tylko pliki .sid (do 64 KB)"); return; }
    String e = player_.play(uploadName_);
    if (e.length()) player_.fs().remove("/" + uploadName_);   // to nie byl poprawny plik SID
    sendResult(e);
}
