#include "DeepSidSource.h"
#if PLAYER_SOURCE_DEEPSID
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <functional>

static const char* DEEPSID_URL = "https://deepsid.chordian.net";
static const char* HVSC_ROOT = "_High Voltage SID Collection";
static const uint32_t NET_RETRY_MS = 5000;      // po bledzie sieci kolejna proba najwczesniej po 5 s
static const uint16_t HTTP_TIMEOUT_MS = 15000;
// Katalog glowny HVSC - API nie listuje katalogu glownego (DOCUMENTS i UPDATE pomijamy).
static const char* const ROOT_DIRS[] = {"DEMOS", "GAMES", "MUSICIANS"};

namespace {

// Strumieniowy czytnik JSON: nie trzyma calej odpowiedzi w pamieci, tylko wywoluje
// cb(glebokosc, klucz, wartosc) dla kazdej wartosci prostej (napis, liczba, true...).
// Glebokosc 1 = obiekt glowny, 2 = obiekt w nim zagniezdzony.
// Stream, bo tego wymaga HTTPClient::writeToStream() - uzywany jest tylko zapis.
class JsonScan : public Stream {
public:
    typedef std::function<void(int depth, const String& key, const String& value)> Callback;
    explicit JsonScan(Callback cb) : cb_(cb) {}
    size_t write(uint8_t c) override { feed(char(c)); return 1; }
    size_t write(const uint8_t* buf, size_t n) override {
        for (size_t i = 0; i < n; i++) feed(char(buf[i]));
        return n;
    }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }

private:
    static const size_t MAX_STR = 512;          // dluzsze napisy (np. opisy STIL) sa obcinane

    void add(char c) { if (str_.length() < MAX_STR) str_ += c; }
    void addCode(uint16_t c) {                  // \uXXXX -> UTF-8
        if (c < 0x80) { add(char(c)); return; }
        if (c < 0x800) { add(char(0xC0 | (c >> 6))); add(char(0x80 | (c & 0x3F))); return; }
        add(char(0xE0 | (c >> 12))); add(char(0x80 | ((c >> 6) & 0x3F))); add(char(0x80 | (c & 0x3F)));
    }
    void flush() {
        if (haveStr_) cb_(depth_, key_, str_);
        else if (tok_.length()) cb_(depth_, key_, tok_);
        haveStr_ = false;
        tok_ = "";
        key_ = "";
    }

    void feed(char c) {
        if (inStr_) {
            if (uni_ >= 0) {
                code_ = code_ * 16 + (isdigit((unsigned char)c) ? c - '0' : (tolower(c) - 'a' + 10));
                if (++uni_ == 4) { addCode(code_); uni_ = -1; }
            } else if (esc_) {
                esc_ = false;
                switch (c) {
                case 'n': add('\n'); break;
                case 't': add('\t'); break;
                case 'r': case 'b': case 'f': break;
                case 'u': uni_ = 0; code_ = 0; break;
                default: add(c);                // \" \\ \/
                }
            } else if (c == '\\') esc_ = true;
            else if (c == '"') { inStr_ = false; haveStr_ = true; }
            else add(c);
            return;
        }
        switch (c) {
        case '"': inStr_ = true; str_ = ""; break;
        case ':': if (haveStr_) { key_ = str_; haveStr_ = false; } break;
        case ',': flush(); break;
        case '{': case '[': depth_++; key_ = ""; haveStr_ = false; break;
        case '}': case ']': flush(); depth_--; break;
        case ' ': case '\t': case '\r': case '\n': break;
        default: if (tok_.length() < 32) tok_ += c;   // liczba, true, false, null
        }
    }

    Callback cb_;
    int depth_ = 0;
    bool inStr_ = false, esc_ = false, haveStr_ = false;
    int uni_ = -1;
    uint16_t code_ = 0;
    String str_, key_, tok_;
};

// Bufor na pobierany plik .sid (tylko zapis, jak JsonScan).
class BufPrint : public Stream {
public:
    BufPrint(uint8_t* buf, size_t cap) : buf_(buf), cap_(cap) {}
    size_t write(uint8_t c) override { return write(&c, 1); }
    size_t write(const uint8_t* b, size_t n) override {
        if (len_ + n > cap_) { overflow_ = true; n = cap_ - len_; }
        memcpy(buf_ + len_, b, n);
        len_ += n;
        return n;
    }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    size_t length() const { return len_; }
    bool overflow() const { return overflow_; }
private:
    uint8_t* buf_;
    size_t cap_, len_ = 0;
    bool overflow_ = false;
};

// Koduje sciezke do adresu URL ("/" zostaje, spacja -> %20).
String urlEncode(const String& s) {
    static const char* hex = "0123456789ABCDEF";
    String out;
    for (size_t i = 0; i < s.length(); i++) {
        unsigned char c = s[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/') out += char(c);
        else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
    }
    return out;
}

// Odpowiedz serwera -> czytnik JSON; zwraca blad albo "".
String readJson(HTTPClient& http, JsonScan& scan) {
    int n = http.writeToStream(&scan);
    if (n < 0) return "DeepSID: " + HTTPClient::errorToString(n);
    return "";
}

// GET przez HTTPS. body() czyta odpowiedz 200 i zwraca blad albo "".
// net = true, gdy blad dotyczy polaczenia (a nie np. braku pliku).
String fetch(const String& url, bool& net, const std::function<String(HTTPClient&)>& body) {
    net = false;
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setConnectTimeout(HTTP_TIMEOUT_MS);
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.useHTTP10(true);                       // bez kodowania chunked i keep-alive
    http.setUserAgent("ESP32-C64-SID-Player");
    if (!http.begin(client, url)) return "zly adres " + url;
    int code = http.GET();
    String e;
    if (code <= 0) { net = true; e = "DeepSID: " + HTTPClient::errorToString(code); }
    else if (code == HTTP_CODE_NOT_FOUND) e = "DeepSID: nie ma takiego pliku";
    else if (code != HTTP_CODE_OK) e = "DeepSID: HTTP " + String(code);
    else {
        e = body(http);
        net = e.length() > 0;
    }
    http.end();
    return e;
}

}  // namespace

// ---------------------------------------------------------------- DeepSidSource

bool DeepSidSource::ready() {
    if (WiFi.status() != WL_CONNECTED) { error_ = "brak polaczenia z internetem"; return false; }
    if (netError_ && millis() - netErrorAt_ < NET_RETRY_MS) return false;
    netError_ = false;
    error_ = "";
    return true;
}

void DeepSidSource::netFailed(const String& why) {
    error_ = why;
    netError_ = true;
    netErrorAt_ = millis();
}

bool DeepSidSource::list(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) {
    out.clear();
    if (!ready()) return false;
    bool found = false;
    if (dir == "/") {
        for (const char* d : ROOT_DIRS) out.push_back({d, 0, true});
        found = true;
    } else {
        bool folders = listFolders(dir, out, maxEntries);
        if (netError_) { out.clear(); return false; }
        bool files = listFiles(dir, out, maxEntries);
        if (netError_) { out.clear(); return false; }
        found = folders || files;
    }
    cachedDir_ = dir;
    cachedDirs_.clear();
    for (const SourceEntry& e : out)
        if (e.dir) cachedDirs_.push_back(e.name);
    return found;
}

// ?folder=X zwraca X i wszystkie foldery pod nim; zostawiamy tylko bezposrednie podfoldery.
bool DeepSidSource::listFolders(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) {
    String prefix = String(HVSC_ROOT) + dir + "/";
    bool found = false;
    JsonScan scan([&](int depth, const String& key, const String& v) {
        if (depth == 1 && key == "folder") found = true;
        if (depth != 2 || key != "folder" || !v.startsWith(prefix) || v.length() <= prefix.length()) return;
        if (v.indexOf('/', prefix.length()) >= 0 || out.size() >= maxEntries) return;
        out.push_back({v.substring(prefix.length()), 0, true});
    });
    bool net;
    String e = fetch(String(DEEPSID_URL) + "/api/v1/?folder=" + urlEncode(dir.substring(1)), net,
                     [&](HTTPClient& http) { return readJson(http, scan); });
    if (e.length()) { if (net) netFailed(e); else error_ = e; return false; }
    return found;
}

// ?file=X/ zwraca pliki w folderze X razem z danymi z bazy DeepSID.
bool DeepSidSource::listFiles(const String& dir, std::vector<SourceEntry>& out, size_t maxEntries) {
    String prefix = String(HVSC_ROOT) + dir + "/";
    bool found = false, current = false;
    JsonScan scan([&](int depth, const String& key, const String& v) {
        if (depth == 1 && key == "folder") found = true;
        if (depth != 2) return;
        if (key == "collection_path") {
            current = v.startsWith(prefix) && v.indexOf('/', prefix.length()) < 0 &&
                      SourcePath::isSid(v) && out.size() < maxEntries;
            if (current) out.push_back({v.substring(prefix.length()), 0, false});
        } else if (current && (key == "data_offset" || key == "data_size")) {
            out.back().size += size_t(v.toInt());   // rozmiar pliku = naglowek + dane
        }
    });
    bool net;
    String e = fetch(String(DEEPSID_URL) + "/api/v1/?file=" + urlEncode(dir.substring(1)) + "/", net,
                     [&](HTTPClient& http) { return readJson(http, scan); });
    if (e.length()) { if (net) netFailed(e); else error_ = e; return false; }
    return found;
}

bool DeepSidSource::isDir(const String& path) {
    if (path == "/") return true;
    // folder z ostatnio wczytanej listy; inne sprawdzi dopiero wczytanie listy
    if (SourcePath::parent(path) == cachedDir_) {
        String name = SourcePath::baseName(path);
        for (const String& d : cachedDirs_)
            if (d == name) return true;
        return false;
    }
    return !SourcePath::isSid(path);
}

String DeepSidSource::read(const String& path, uint8_t*& buf, size_t& len, size_t maxLen) {
    buf = nullptr;
    len = 0;
    if (!ready()) return error();
    bool net;
    String e = fetch(String(DEEPSID_URL) + "/music/" + urlEncode(String(HVSC_ROOT) + path), net,
                     [&](HTTPClient& http) -> String {
                         int size = http.getSize();          // -1 = serwer nie podal dlugosci
                         if (size > int(maxLen)) return "plik za duzy";
                         size_t cap = size > 0 ? size_t(size) : maxLen;
                         buf = (uint8_t*)malloc(cap);
                         if (!buf) return "brak pamieci";
                         BufPrint bp(buf, cap);
                         int n = http.writeToStream(&bp);
                         if (bp.overflow()) return "plik za duzy";
                         if (n < 0 || !bp.length()) return "DeepSID: blad pobierania " + path;
                         len = bp.length();
                         return "";
                     });
    if (e.length()) {
        free(buf);
        buf = nullptr;
        len = 0;
        // "plik za duzy" i "brak pamieci" to nie bledy sieci
        if (net && e != "plik za duzy" && e != "brak pamieci") netFailed(e);
    }
    return e;
}
#endif
