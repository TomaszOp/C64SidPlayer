/**
 * @file Player.cpp
 * @brief Implementation of Player: I2S audio task, sources and folders, file list and playback control.
 */
#include "Player.h"
#include <algorithm>
#include <driver/i2s.h>
#include <soc/soc_caps.h>

#if defined(SID_OUTPUT_INTERNAL_DAC) && !SOC_I2S_SUPPORTS_DAC
#error "Ten uklad nie ma wbudowanego DAC - usun SID_OUTPUT_INTERNAL_DAC i uzyj DAC I2S"
#endif

static const int AUDIO_CHUNK = 256;   ///< Samples per audio block (also the I2S DMA buffer length).

Player::Player()
    : mutex_(nullptr), state_(PlayerState::Stopped), muted_(false), testTone_(false),
      volume_(80), renderUs_(0), modelSetting_(-1), i2sError_(0), fileSource_(SourceType::Flash),
      source_(SourceType::Flash), listValid_(false), truncated_(false) {
    for (uint8_t i = 0; i < SOURCE_TYPE_COUNT; i++) {
        sources_[i] = nullptr;
        dir_[i] = "/";
    }
}

void Player::lock() { xSemaphoreTake((SemaphoreHandle_t)mutex_, portMAX_DELAY); }
void Player::unlock() { xSemaphoreGive((SemaphoreHandle_t)mutex_); }

bool Player::begin(const PlayerConfig& cfg) {
    cfg_ = cfg;
    setVolume(cfg.volume);
    mutex_ = xSemaphoreCreateMutex();
    sid_.init(cfg.sampleRate);
    // zrodla plikow z tablicy PLAYER_SOURCES; brak karty SD nie jest bledem
    for (uint8_t i = 0; i < PLAYER_SOURCE_COUNT; i++) {
        SourceType t = PLAYER_SOURCES[i].type;
        sources_[uint8_t(t)] = sourceImpl(t);
        if (sources_[uint8_t(t)]) sources_[uint8_t(t)]->begin(cfg_);
    }
    bool fsOk = sources_[uint8_t(SourceType::Flash)]->ready();
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

// ---------------------------------------------------------------- zrodla i foldery

String Player::setSource(SourceType type) {
    if (uint8_t(type) >= SOURCE_TYPE_COUNT || !sources_[uint8_t(type)])
        return "to zrodlo nie jest wkompilowane";
    source_ = type;
    listValid_ = false;
    if (!sourceReady()) return sourceError();
    return "";
}

const SourceInfo& Player::sourceInfo() const { return *::sourceInfo(source_); }
bool Player::sourceReady() { return activeSource().ready(); }
String Player::sourceError() { return activeSource().error(); }

String Player::path(const String& nameOrPath) const {
    return nameOrPath.startsWith("/") ? nameOrPath : SourcePath::join(dir(), nameOrPath);
}

String Player::cd(const String& to) {
    String target;
    if (!to.length() || to == "/") target = "/";
    else if (to == "..") target = SourcePath::parent(dir());
    else target = path(to);
    if (target.length() > 1 && target.endsWith("/")) target.remove(target.length() - 1);
    if (!SourcePath::valid(target)) return "zla nazwa folderu";
    if (!sourceReady()) return sourceError();
    if (!activeSource().isDir(target)) return "nie ma folderu " + target;
    dir_[uint8_t(source_)] = target;
    listValid_ = false;
    return "";
}

void Player::setDir(SourceType type, const String& d) {
    if (uint8_t(type) >= SOURCE_TYPE_COUNT) return;
    dir_[uint8_t(type)] = SourcePath::valid(d) ? d : String("/");
    if (type == source_) listValid_ = false;
}

// ---------------------------------------------------------------- pliki

const std::vector<PlayerFile>& Player::files() {
    if (listValid_) return list_;
    truncated_ = false;
    if (!sourceReady() || !activeSource().list(dir(), list_, MAX_LIST)) {
        list_.clear();
        // folder zniknal (np. inna karta SD) - wroc do katalogu glownego
        if (sourceReady() && dir() != "/") {
            dir_[uint8_t(source_)] = "/";
            activeSource().list("/", list_, MAX_LIST);
        }
    }
    truncated_ = list_.size() >= MAX_LIST;
    std::sort(list_.begin(), list_.end(), [](const PlayerFile& a, const PlayerFile& b) {
        if (a.dir != b.dir) return a.dir;
        return strcasecmp(a.name.c_str(), b.name.c_str()) < 0;
    });
    listValid_ = true;
    return list_;
}

void Player::refreshFiles() { listValid_ = false; }

int Player::dirCount() {
    const std::vector<PlayerFile>& l = files();
    int n = 0;
    while (n < int(l.size()) && l[n].dir) n++;
    return n;
}

int Player::fileCount() { return int(files().size()) - dirCount(); }

String Player::fileName(int index) {
    int d = dirCount();
    if (index < 1 || index > int(list_.size()) - d) return "";
    return list_[d + index - 1].name;
}

int Player::fileIndex(const String& nameOrPath) {
    String p = path(nameOrPath);
    if (SourcePath::parent(p) != dir()) return 0;
    String name = SourcePath::baseName(p);
    int d = dirCount();
    for (size_t i = d; i < list_.size(); i++)
        if (list_[i].name == name) return int(i) - d + 1;
    return 0;
}

bool Player::removeFile(const String& nameOrPath) {
    String p = path(nameOrPath);
    if (!SourcePath::valid(p) || !SourcePath::isSid(p)) return false;
    return removePath(p);
}

bool Player::removePath(const String& p) {
    if (!activeSource().canWrite()) return false;
    if (p == file_ && fileSource_ == source_) stop();
    bool ok = activeSource().remove(p);
    listValid_ = false;
    return ok;
}

uint64_t Player::freeSpace() { return activeSource().freeSpace(); }
uint64_t Player::totalSpace() { return activeSource().totalSpace(); }

bool Player::uploadAllowed() {
    return sourceInfo().upload && activeSource().canWrite() && sourceReady();
}

bool Player::deleteAllowed() {
    return activeSource().canWrite() && sourceReady();
}

String Player::prepareUpload(const String& fileName, String& p) {
    p = "";
    if (!uploadAllowed()) return String("wgrywanie do: ") + sourceInfo().name + " jest wylaczone";
    String clean = sanitizeName(fileName);
    if (!clean.length()) return "dozwolone sa tylko pliki .sid";
    String full = SourcePath::join(dir(), clean);
    if (full.length() > activeSource().maxPathLength())
        return "za dluga sciezka (max " + String(activeSource().maxPathLength()) + " znakow)";
    if (full == file_ && fileSource_ == source_) stop();
    p = full;
    return "";
}

fs::File Player::openWrite(const String& p) {
    listValid_ = false;
    return activeSource().openWrite(p);
}

String Player::sanitizeName(const String& name) {
    String n = SourcePath::baseName(name);
    String out;
    for (size_t i = 0; i < n.length(); i++) {
        char c = n[i];
        if (isalnum((unsigned char)c) || c == '.' || c == '-' || c == '_') out += c;
        else if (c == ' ') out += '_';
    }
    if (out.length() > 60) out = out.substring(out.length() - 60);
    if (!out.length() || out[0] == '.' || !SourcePath::isSid(out)) return "";
    return out;
}

// ---------------------------------------------------------------- odtwarzanie

String Player::loadAndStart(const String& p, int song) {
    uint8_t* buf = nullptr;
    size_t len = 0;
    String err = activeSource().read(p, buf, len, MAX_FILE_SIZE);
    if (err.length()) return err;

    lock();
    state_ = PlayerState::Stopped;
    sid_.setModelOverride(modelSetting_);
    if (sid_.load(buf, len) && sid_.startSong(song)) {
        file_ = p;
        fileSource_ = source_;
        state_ = PlayerState::Playing;
    } else {
        err = sid_.error();
    }
    unlock();
    free(buf);
    return err;
}

String Player::play(const String& nameOrPath, int song) {
    String p = path(nameOrPath);
    if (!SourcePath::valid(p) || !SourcePath::isSid(p)) return "zla nazwa pliku (dozwolone tylko .sid)";
    if (!sourceReady()) return sourceError();
    return loadAndStart(p, song);
}

String Player::playIndex(int index, int song) {
    String name = fileName(index);
    if (!name.length()) return "nie ma pliku nr " + String(index);
    return loadAndStart(SourcePath::join(dir(), name), song);
}

// next / previous file in the current folder (folders are skipped)
String Player::stepFile(int delta) {
    int n = fileCount();
    if (!n) return "brak plikow w folderze " + dir();
    int i = fileSource_ == source_ ? fileIndex(file_) : 0;
    if (!i) i = delta > 0 ? 0 : n + 1;      // biezacy plik nie jest w tym folderze
    i += delta;
    if (i > n) i = 1;
    if (i < 1) i = n;
    return playIndex(i);
}

String Player::nextFile() { return stepFile(1); }
String Player::prevFile() { return stepFile(-1); }

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
    s.fileSource = fileSource_;
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
    s.source = source_;
    s.dir = dir();
    return s;
}
