#include "Player.h"
#include <LittleFS.h>
#include <algorithm>
#include <driver/i2s.h>
#include <soc/soc_caps.h>

#if defined(SID_OUTPUT_INTERNAL_DAC) && !SOC_I2S_SUPPORTS_DAC
#error "Ten uklad nie ma wbudowanego DAC - usun SID_OUTPUT_INTERNAL_DAC i uzyj DAC I2S"
#endif

static const int AUDIO_CHUNK = 256;

Player::Player()
    : mutex_(nullptr), state_(PlayerState::Stopped), muted_(false), testTone_(false),
      volume_(80), renderUs_(0), modelSetting_(-1), i2sError_(0) {}

void Player::lock() { xSemaphoreTake((SemaphoreHandle_t)mutex_, portMAX_DELAY); }
void Player::unlock() { xSemaphoreGive((SemaphoreHandle_t)mutex_); }

bool Player::begin(const PlayerConfig& cfg) {
    cfg_ = cfg;
    setVolume(cfg.volume);
    mutex_ = xSemaphoreCreateMutex();
    sid_.init(cfg.sampleRate);
    bool fsOk = LittleFS.begin(cfg.formatFsIfFailed);
    bool audio = setupI2S();
    xTaskCreatePinnedToCore(audioTaskEntry, "sid-audio", 4096, this, cfg.audioPriority, nullptr, cfg.audioCore);
    return fsOk && audio;
}

const char* Player::audioError() const {
    return i2sError_ ? esp_err_to_name(i2sError_) : "OK";
}

// ---------------------------------------------------------------- audio

bool Player::setupI2S() {
    i2s_config_t c = {};
#ifdef SID_OUTPUT_INTERNAL_DAC
    c.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN);
    c.communication_format = I2S_COMM_FORMAT_STAND_MSB;
    c.use_apll = false;
#else
    c.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    c.communication_format = I2S_COMM_FORMAT_STAND_I2S;
#if SOC_I2S_SUPPORTS_APLL
    c.use_apll = true;     // dokladniejszy zegar audio (ESP32-S3 nie ma APLL)
#endif
#endif
    c.sample_rate = cfg_.sampleRate;
    c.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    c.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    c.intr_alloc_flags = 0;
    c.dma_buf_count = 8;
    c.dma_buf_len = AUDIO_CHUNK;
    c.tx_desc_auto_clear = true;
    esp_err_t e = i2s_driver_install(I2S_NUM_0, &c, 0, nullptr);
#ifdef SID_OUTPUT_INTERNAL_DAC
    if (e == ESP_OK) e = i2s_set_pin(I2S_NUM_0, nullptr);
    if (e == ESP_OK) e = i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
#else
    i2s_pin_config_t pins = {};
    pins.mck_io_num = I2S_PIN_NO_CHANGE;
    pins.bck_io_num = cfg_.pinBck;
    pins.ws_io_num = cfg_.pinWs;
    pins.data_out_num = cfg_.pinData;
    pins.data_in_num = I2S_PIN_NO_CHANGE;
    if (e == ESP_OK) e = i2s_set_pin(I2S_NUM_0, &pins);
#endif
    i2sError_ = e;
    return e == ESP_OK;
}

void Player::audioTaskEntry(void* arg) { static_cast<Player*>(arg)->audioTask(); }

void Player::audioTask() {
    static int16_t mono[AUDIO_CHUNK];
    static uint16_t frames[AUDIO_CHUNK * 2];
    float phase = 0;
    for (;;) {
        uint32_t t0 = micros();
        float gain = muted_ ? 0.0f : volume_ / 100.0f;
        if (testTone_) {
            const float step = 2.0f * float(M_PI) * 440.0f / cfg_.sampleRate;
            for (int i = 0; i < AUDIO_CHUNK; i++) {
                mono[i] = int16_t(sinf(phase) * 16000.0f * gain);
                phase += step;
                if (phase > 2.0f * float(M_PI)) phase -= 2.0f * float(M_PI);
            }
        } else {
            lock();
            // przy wyciszeniu muzyka gra dalej (czas plynie), przy pauzie emulacja stoi
            if (state_ == PlayerState::Playing) sid_.render(mono, AUDIO_CHUNK, gain);
            else memset(mono, 0, sizeof(mono));
            unlock();
        }
        renderUs_ = micros() - t0;

        for (int i = 0; i < AUDIO_CHUNK; i++) {
#ifdef SID_OUTPUT_INTERNAL_DAC
            uint16_t s = uint16_t(int32_t(mono[i]) + 32768);   // DAC przyjmuje wartosci bez znaku
#else
            uint16_t s = uint16_t(mono[i]);
#endif
            frames[i * 2] = s;
            frames[i * 2 + 1] = s;
        }
        size_t written;
        i2s_write(I2S_NUM_0, frames, sizeof(frames), &written, portMAX_DELAY);
    }
}

unsigned Player::cpuLoad() const {
    // blok AUDIO_CHUNK probek trwa AUDIO_CHUNK/sampleRate s; >100% = ESP32 nie nadaza
    return unsigned(renderUs_ * 100ULL * cfg_.sampleRate / (AUDIO_CHUNK * 1000000ULL));
}

// ---------------------------------------------------------------- pliki

fs::FS& Player::fs() { return LittleFS; }

std::vector<PlayerFile> Player::files() {
    std::vector<PlayerFile> list;
    File root = LittleFS.open("/");
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
        if (f.isDirectory()) continue;
        String name = f.name();
        if (name.startsWith("/")) name = name.substring(1);
        list.push_back({name, f.size()});
    }
    std::sort(list.begin(), list.end(), [](const PlayerFile& a, const PlayerFile& b) {
        return strcasecmp(a.name.c_str(), b.name.c_str()) < 0;
    });
    return list;
}

int Player::fileCount() { return int(files().size()); }

String Player::fileName(int index) {
    std::vector<PlayerFile> list = files();
    if (index < 1 || index > int(list.size())) return "";
    return list[index - 1].name;
}

int Player::fileIndex(const String& name) {
    std::vector<PlayerFile> list = files();
    for (size_t i = 0; i < list.size(); i++)
        if (list[i].name == name) return int(i) + 1;
    return 0;
}

bool Player::removeFile(const String& name) {
    String clean = sanitizeName(name);
    if (!clean.length()) return false;
    if (clean == file_) stop();
    return LittleFS.remove(path(clean));
}

size_t Player::freeSpace() { return LittleFS.totalBytes() - LittleFS.usedBytes(); }

String Player::sanitizeName(const String& name) {
    String n = name;
    int slash = n.lastIndexOf('/');
    if (slash >= 0) n = n.substring(slash + 1);
    String out;
    for (size_t i = 0; i < n.length(); i++) {
        char c = n[i];
        if (isalnum((unsigned char)c) || c == '.' || c == '-' || c == '_') out += c;
        else if (c == ' ') out += '_';
    }
    if (out.length() > 60) out = out.substring(out.length() - 60);
    String lower = out;
    lower.toLowerCase();
    if (!lower.endsWith(".sid")) return "";
    return out;
}

// ---------------------------------------------------------------- odtwarzanie

String Player::loadAndStart(const String& name, int song) {
    File f = LittleFS.open(path(name), "r");
    if (!f) return "nie ma pliku " + name;
    size_t len = f.size();
    if (len > MAX_FILE_SIZE) { f.close(); return "plik za duzy"; }
    uint8_t* buf = (uint8_t*)malloc(len);
    if (!buf) { f.close(); return "brak pamieci"; }
    f.read(buf, len);
    f.close();

    String err;
    lock();
    state_ = PlayerState::Stopped;
    sid_.setModelOverride(modelSetting_);
    if (sid_.load(buf, len) && sid_.startSong(song)) {
        file_ = name;
        state_ = PlayerState::Playing;
    } else {
        err = sid_.error();
    }
    unlock();
    free(buf);
    return err;
}

String Player::play(const String& name, int song) {
    String clean = sanitizeName(name);
    if (!clean.length()) return "zla nazwa pliku (dozwolone tylko .sid)";
    return loadAndStart(clean, song);
}

String Player::playIndex(int index, int song) {
    String name = fileName(index);
    if (!name.length()) return "nie ma pliku nr " + String(index);
    return loadAndStart(name, song);
}

String Player::nextFile() {
    int n = fileCount();
    if (!n) return "brak plikow";
    int i = fileIndex(file_) + 1;
    return playIndex(i > n ? 1 : i);
}

String Player::prevFile() {
    int n = fileCount();
    if (!n) return "brak plikow";
    int i = fileIndex(file_) - 1;
    return playIndex(i < 1 ? n : i);
}

void Player::stop() {
    lock();
    state_ = PlayerState::Stopped;
    unlock();
}

void Player::pause() {
    if (state_ == PlayerState::Playing) state_ = PlayerState::Paused;
}

void Player::resume() {
    if (state_ == PlayerState::Paused) { state_ = PlayerState::Playing; return; }
    // po stop(): zacznij biezacy podutwor od poczatku
    if (state_ == PlayerState::Stopped && sid_.loaded()) restartSong();
}

void Player::togglePause() {
    if (state_ == PlayerState::Playing) pause();
    else resume();
}

String Player::setSong(int song) {
    if (!sid_.loaded()) return "brak utworu";
    lock();
    int songs = sid_.info().songs;
    if (song < 1) song = songs;
    if (song > songs) song = 1;
    sid_.setModelOverride(modelSetting_);
    sid_.startSong(song);
    state_ = PlayerState::Playing;
    unlock();
    return "";
}

String Player::nextSong() { return setSong(sid_.currentSong() + 1); }
String Player::prevSong() { return setSong(sid_.currentSong() - 1); }
String Player::restartSong() { return setSong(sid_.currentSong()); }

// ---------------------------------------------------------------- dzwiek

void Player::setVolume(int percent) {
    volume_ = constrain(percent, 0, 100);
}

String Player::setModel(int model) {
    modelSetting_ = constrain(model, -1, 1);
    // nowy model wymaga ponownego uruchomienia podutworu
    if (sid_.loaded() && state_ != PlayerState::Stopped) return restartSong();
    return "";
}

// ---------------------------------------------------------------- stan

PlayerStatus Player::status() {
    PlayerStatus s;
    lock();
    const SidInfo& i = sid_.info();
    s.state = state_;
    s.loaded = sid_.loaded();
    s.file = file_;
    s.title = s.loaded ? String(i.name) : "";
    s.author = s.loaded ? String(i.author) : "";
    s.released = s.loaded ? String(i.released) : "";
    s.song = sid_.currentSong();
    s.songs = s.loaded ? i.songs : 0;
    s.ms = sid_.playedMs();
    s.rsid = i.rsid;
    s.ntsc = i.ntsc;
    s.sidModel = i.model == Sid::MOS8580 ? 1 : 0;
    s.frameHz = s.loaded ? sid_.frameRateHz() : 0;
    unlock();
    s.volume = volume_;
    s.muted = muted_;
    s.modelSetting = modelSetting_;
    s.testTone = testTone_;
    s.cpuLoad = cpuLoad();
    s.engine = SID_ENGINE_NAME;
    return s;
}
