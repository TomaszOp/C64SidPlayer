/**
 * @file PlayerLcd.cpp
 * @brief Implementation of PlayerLcd: focus / edit logic, list browsing and drawing.
 */
#include "PlayerLcd.h"
#if USE_LCD

// uklad ekranu 320x240 (poziomo)
static const int W = 320;                       ///< Screen width.
static const int NOW_Y = 0, NOW_H = 60;         ///< "Now playing" field.
static const int VOL_Y = 60, VOL_H = 22;        ///< Volume field.
static const int SRC_Y = 82, SRC_H = 22;        ///< Source field.
static const int PATH_Y = 104, PATH_H = 18;     ///< Path / message bar.
static const int LIST_Y = 122, ROW_H = 19;      ///< List: top and row height.
static const int LIST_ROWS = 6;                 ///< Visible list rows.
static const int NAME_X = 16;                   ///< List: x of the names (room for the ">" marker).

static const uint32_t POLL_MS = 100;            ///< How often the player status is read.
static const uint32_t EDIT_TIMEOUT_MS = 8000;   ///< Edit mode ends after this time without input.
static const uint32_t SRC_CHECK_MS = 2000;      ///< How often the source readiness is checked.
static const uint32_t SCROLL_DELAY_MS = 1000;   ///< A long highlighted name starts scrolling after this.
static const uint32_t SCROLL_STEP_MS = 100;     ///< Scroll step interval.

PlayerLcd::PlayerLcd(Player& player) : player_(player), spr_(&tft_) {}

const LcdTheme& PlayerLcd::theme() const {
    return LCD_THEMES[settings_.theme < uint8_t(LcdThemeId::Count) ? settings_.theme : 0];
}

void PlayerLcd::begin(const LcdConfig& cfg) {
    cfg_ = cfg;
    settings_.load();
    input_.begin(cfg.input);
    if (cfg.pinBacklight >= 0) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcAttachChannel(cfg.pinBacklight, 5000, 8, cfg.ledcChannel);
#else
        ledcSetup(cfg.ledcChannel, 5000, 8);
        ledcAttachPin(cfg.pinBacklight, cfg.ledcChannel);
#endif
        backlight(0);   // bez smieci na ekranie podczas startu
    }
    tft_.init();
    tft_.setRotation(cfg.rotation);
    spr_.setColorDepth(16);
    sprOk_ = spr_.createSprite(W, NOW_H) != nullptr;
    if (!sprOk_) log_e("LCD: brak pamieci na sprite");
    started_ = true;
    applySettings();
    pollStatus();
    lastPoll_ = millis();
    if (rowCount() > 1 && hasUpRow()) cursor_ = 1;
    int r = playingRow();
    if (r >= 0) setCursor(r);
    draw();
    backlight(settings_.brightness);
}

void PlayerLcd::applySettings() {
    if (settings_.brightness < 5) settings_.brightness = 5;
    if (settings_.brightness > 100) settings_.brightness = 100;
    if (started_) tft_.fillScreen(theme().bg);
    dirty_ = D_ALL;
    if (!asleep_) backlight(settings_.brightness);
}

void PlayerLcd::message(const String& text, uint32_t ms) {
    msg_ = text;
    msgUntil_ = millis() + ms;
    if (!msgUntil_) msgUntil_ = 1;
    dirty_ |= D_PATH;
}

void PlayerLcd::handle() {
    if (!started_) return;
    for (int i = 0; i < 8; i++) {
        LcdEvent e = input_.poll();
        if (e == LcdEvent::None) break;
        onEvent(e);
    }
    if (millis() - lastPoll_ >= POLL_MS) {
        lastPoll_ = millis();
        pollStatus();
    }
    idleTasks();
    draw();
}

// ---------------------------------------------------------------- sterowanie

void PlayerLcd::onEvent(LcdEvent e) {
    if (asleep_) {   // pierwszy ruch tylko budzi ekran
        asleep_ = false;
        backlight(settings_.brightness);
        return;
    }
    switch (e) {
    case LcdEvent::Turn: onTurn(input_.turnSteps(), input_.turnFast()); break;
    case LcdEvent::Select: onSelect(); break;
    case LcdEvent::Back: onBack(); break;
    case LcdEvent::Home:
        endEdit();
        focus(Field::Now);
        msgUntil_ = 0; msg_ = ""; dirty_ |= D_PATH;
        break;
    case LcdEvent::PlayPause: togglePause(); break;
    case LcdEvent::Stop:
        player_.stop();
        lastPoll_ = 0;
        break;
    case LcdEvent::HoldHint: message("Pusc: Wstecz, trzymaj: Start", 1500); break;
    default: break;
    }
}

void PlayerLcd::onTurn(int steps, bool fast) {
    if (edit_) {
        switch (field_) {
        case Field::Now:
            if (last_.loaded) player_.setSong(last_.song + steps);
            lastPoll_ = 0;
            break;
        case Field::Volume:
            player_.setVolume(player_.volume() + steps * (fast ? 5 : 1));
            dirty_ |= D_VOL;
            break;
        case Field::Source: {
            int n = PLAYER_SOURCE_COUNT;
            srcPick_ = uint8_t(((srcPick_ + steps) % n + n) % n);
            dirty_ |= D_SRC;
            break;
        }
        default: break;
        }
        return;
    }
    if (field_ == Field::List) {
        // obrot w gore na pierwszym wierszu = wyjscie z listy na pole Zrodlo
        if (cursor_ == 0 && steps < 0) { focus(Field::Source); return; }
        setCursor(cursor_ + steps * (fast ? 5 : 1));
        return;
    }
    int f = int(field_) + steps;
    if (f < int(Field::Now)) f = int(Field::Now);
    if (f > int(Field::List)) f = int(Field::List);
    focus(Field(f));
}

void PlayerLcd::onSelect() {
    switch (field_) {
    case Field::Now:
        if (input_.encoderOnly()) togglePause();   // bez przyciskow: Wybierz na Teraz gra = pauza
        else { edit_ = !edit_; dirty_ |= D_NOW; }
        break;
    case Field::Volume:
        edit_ = !edit_;
        dirty_ |= D_VOL;
        break;
    case Field::Source:
        if (!edit_) {
            edit_ = true;
            srcPick_ = 0;
            for (uint8_t i = 0; i < PLAYER_SOURCE_COUNT; i++)
                if (PLAYER_SOURCES[i].type == player_.source()) srcPick_ = i;
            dirty_ |= D_SRC;
        } else {
            // zrodlo zmienia sie dopiero po zatwierdzeniu (DeepSid wczytuje liste przez internet)
            edit_ = false;
            SourceType type = PLAYER_SOURCES[srcPick_].type;
            if (type != player_.source()) {
                loadingNote(type);
                String err = player_.setSource(type);
                player_.files();
                listGen_ = player_.listGeneration();
                if (err.length()) message(err);
                cursor_ = (hasUpRow() && rowCount() > 1) ? 1 : 0;
                top_ = 0;
                lastSrcCheck_ = 0;
            }
            dirty_ |= D_SRC | D_LIST | D_PATH;
            focus(Field::List);
        }
        break;
    case Field::List: openRow(); break;
    }
}

void PlayerLcd::onBack() {
    if (edit_) { endEdit(); return; }
    if (field_ == Field::List && !goUp()) focus(Field::Source);
}

void PlayerLcd::togglePause() {
    if (!last_.loaded) { message("Brak utworu - wybierz plik z listy"); return; }
    player_.togglePause();
    lastPoll_ = 0;
}

void PlayerLcd::endEdit() {
    if (!edit_) return;
    edit_ = false;
    dirty_ |= field_ == Field::Now ? D_NOW : field_ == Field::Volume ? D_VOL : D_SRC;
}

void PlayerLcd::focus(Field f) {
    if (f == field_) return;
    static const uint8_t bits[] = {D_NOW, D_VOL, D_SRC, D_LIST | D_PATH};
    endEdit();
    dirty_ |= bits[uint8_t(field_)] | bits[uint8_t(f)];
    field_ = f;
    cursorSince_ = millis();
    scrollPos_ = 0;
}

// ---------------------------------------------------------------- lista

bool PlayerLcd::hasUpRow() { return player_.dir() != "/"; }

int PlayerLcd::rowCount() {
    int n = int(player_.files().size());   // najpierw files() - moze wrocic do "/"
    return n + (hasUpRow() ? 1 : 0);
}

const PlayerFile* PlayerLcd::rowEntry(int row) {
    const std::vector<PlayerFile>& l = player_.files();
    int i = row - (hasUpRow() ? 1 : 0);
    return i >= 0 && i < int(l.size()) ? &l[i] : nullptr;
}

int PlayerLcd::findRow(const String& name) {
    const std::vector<PlayerFile>& l = player_.files();
    int up = hasUpRow() ? 1 : 0;
    for (size_t i = 0; i < l.size(); i++)
        if (l[i].name == name) return int(i) + up;
    return -1;
}

int PlayerLcd::playingRow() {
    if (!haveLast_ || !last_.loaded || last_.fileSource != player_.source()) return -1;
    if (SourcePath::parent(last_.file) != player_.dir()) return -1;
    return findRow(SourcePath::baseName(last_.file));
}

void PlayerLcd::setCursor(int row) {
    int n = rowCount();
    if (row >= n) row = n - 1;
    if (row < 0) row = 0;
    if (row == cursor_) return;
    int old = cursor_, oldTop = top_;
    cursor_ = row;
    if (cursor_ < top_) top_ = cursor_;
    if (cursor_ >= top_ + LIST_ROWS) top_ = cursor_ - LIST_ROWS + 1;
    if (top_ != oldTop || rowA_ >= 0) dirty_ |= D_LIST;
    else { rowA_ = old; rowB_ = cursor_; }
    dirty_ |= D_PATH;
    cursorSince_ = millis();
    scrollPos_ = 0;
}

void PlayerLcd::loadingNote(SourceType type) {
    if (type != SourceType::DeepSid || !sprOk_) return;
    message("Wczytywanie...", 500);
    drawPath();   // od razu - nastepne wywolanie Playera blokuje na kilka sekund
}

String PlayerLcd::changeDir(const String& to) {
    loadingNote(player_.source());
    String err = player_.cd(to);
    player_.files();
    listGen_ = player_.listGeneration();
    dirty_ |= D_LIST | D_PATH;
    return err;
}

void PlayerLcd::openRow() {
    if (cursor_ >= rowCount()) return;
    const PlayerFile* e = rowEntry(cursor_);
    if (!e) { goUp(); return; }   // ".."
    String name = e->name;        // kopia - cd() wczytuje nowa liste
    if (e->dir) {
        String err = changeDir(name);
        if (err.length()) { message(err); return; }
        top_ = 0;
        cursor_ = rowCount() > 1 ? 1 : 0;   // pierwszy wpis pod ".."
        scrollPos_ = 0;
        cursorSince_ = millis();
        return;
    }
    loadingNote(player_.source());
    String err = player_.play(name);
    if (err.length()) message(err);
    lastPoll_ = 0;
    dirty_ |= D_NOW | D_LIST;
}

bool PlayerLcd::goUp() {
    if (!hasUpRow()) return false;
    String from = SourcePath::baseName(player_.dir());
    String err = changeDir("..");
    if (err.length()) { message(err); return true; }
    int r = findRow(from);   // podswietl folder, z ktorego wyszlismy
    cursor_ = r < 0 ? 0 : r;
    int n = rowCount();
    top_ = cursor_ - LIST_ROWS / 2;
    if (top_ > n - LIST_ROWS) top_ = n - LIST_ROWS;
    if (top_ < 0) top_ = 0;
    scrollPos_ = 0;
    cursorSince_ = millis();
    return true;
}

// ---------------------------------------------------------------- stan

void PlayerLcd::pollStatus() {
    PlayerStatus s = player_.status();
    player_.files();   // wczytuje liste ponownie, gdy WWW / konsola zmienily folder lub pliki
    bool listChanged = player_.listGeneration() != listGen_;
    if (!haveLast_) {
        dirty_ = D_ALL;
    } else {
        if (s.title != last_.title || s.author != last_.author || s.released != last_.released ||
            s.state != last_.state || s.loaded != last_.loaded || s.song != last_.song ||
            s.songs != last_.songs || s.sidModel != last_.sidModel || s.ntsc != last_.ntsc ||
            s.rsid != last_.rsid || s.ms / 1000 != last_.ms / 1000)
            dirty_ |= D_NOW;
        if (s.volume != last_.volume || s.muted != last_.muted) dirty_ |= D_VOL;
        if (s.source != last_.source) { dirty_ |= D_SRC | D_LIST | D_PATH; lastSrcCheck_ = 0; }
        if (s.dir != last_.dir) dirty_ |= D_LIST | D_PATH;
        if (s.file != last_.file || s.fileSource != last_.fileSource) dirty_ |= D_LIST;
    }
    bool fileChanged = !haveLast_ || s.file != last_.file;
    last_ = s;
    haveLast_ = true;
    if (listChanged) {
        listGen_ = player_.listGeneration();
        dirty_ |= D_LIST | D_PATH;
        int n = rowCount();
        if (cursor_ >= n) cursor_ = n ? n - 1 : 0;
        if (top_ > cursor_) top_ = cursor_;
    }
    // plik zmienil sie sam (WWW, konsola) - przewin liste, gdy nie jest zaznaczona
    if (fileChanged && field_ != Field::List) {
        int r = playingRow();
        if (r >= 0) setCursor(r);
    }
    if (millis() - lastSrcCheck_ >= SRC_CHECK_MS || !lastSrcCheck_) {
        lastSrcCheck_ = millis();
        bool ready = player_.sourceReady();
        String err = ready ? String() : player_.sourceError();
        if (ready != srcReady_ || err != srcError_) dirty_ |= D_SRC;
        srcReady_ = ready;
        srcError_ = err;
    }
}

void PlayerLcd::idleTasks() {
    uint32_t now = millis();
    uint32_t idle = now - input_.lastActivity();
    if (edit_ && idle > EDIT_TIMEOUT_MS) endEdit();
    if (msgUntil_ && int32_t(now - msgUntil_) >= 0) {
        msgUntil_ = 0;
        msg_ = "";
        dirty_ |= D_PATH;
    }
    // wygaszanie: przyciemnienie, potem wylaczenie podswietlenia
    uint8_t target = settings_.brightness;
    if (settings_.offSec && idle > uint32_t(settings_.offSec) * 1000) target = 0;
    else if (settings_.dimSec && idle > uint32_t(settings_.dimSec) * 1000)
        target = settings_.dimLevel < target ? settings_.dimLevel : target;
    asleep_ = target < settings_.brightness;
    backlight(target);
    // przewijanie dlugiej nazwy w podswietlonym wierszu
    if (field_ == Field::List && scrollMax_ > 0 && now - cursorSince_ > SCROLL_DELAY_MS &&
        now - lastScroll_ >= SCROLL_STEP_MS) {
        lastScroll_ = now;
        scrollPos_ += 3;
        if (scrollPos_ > scrollMax_ + 30) { scrollPos_ = 0; cursorSince_ = now; }   // pauza na koncu
        if (rowA_ < 0) rowA_ = cursor_;
        else dirty_ |= D_LIST;
    }
}

void PlayerLcd::backlight(uint8_t percent) {
    if (cfg_.pinBacklight < 0 || percent == blNow_) return;
    blNow_ = percent;
    uint32_t duty = uint32_t(percent) * 255 / 100;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(cfg_.pinBacklight, duty);
#else
    ledcWrite(cfg_.ledcChannel, duty);
#endif
}

// ---------------------------------------------------------------- rysowanie

void PlayerLcd::draw() {
    if (!sprOk_) { dirty_ = 0; rowA_ = rowB_ = -1; return; }
    if (dirty_ & D_NOW) drawNow();
    if (dirty_ & D_VOL) drawVolume();
    if (dirty_ & D_SRC) drawSource();
    if (dirty_ & D_PATH) drawPath();
    if ((dirty_ & D_LIST) || rowA_ >= 0 || rowB_ >= 0) {
        playRow_ = playingRow();
        if (dirty_ & D_LIST) {
            scrollMax_ = 0;
            for (int i = 0; i < LIST_ROWS; i++) drawRow(i);
        } else {
            if (rowA_ >= top_ && rowA_ < top_ + LIST_ROWS) drawRow(rowA_ - top_);
            if (rowB_ != rowA_ && rowB_ >= top_ && rowB_ < top_ + LIST_ROWS) drawRow(rowB_ - top_);
        }
    }
    dirty_ = 0;
    rowA_ = rowB_ = -1;
}

void PlayerLcd::band(int h, uint16_t bg) {
    spr_.fillRect(0, 0, W, h, bg);
    spr_.setTextDatum(TL_DATUM);
    spr_.setTextFont(2);
}

void PlayerLcd::pushField(Field f, int y, int h) {
    if (field_ == f) {
        const uint16_t c = theme().accent;
        spr_.drawRect(0, 0, W, h, c);
        spr_.drawRect(1, 1, W - 2, h - 2, c);
    }
    spr_.pushSprite(0, y, 0, 0, W, h);
}

void PlayerLcd::drawNow() {
    const LcdTheme& t = theme();
    const PlayerStatus& s = last_;
    bool ed = edit_ && field_ == Field::Now;
    band(NOW_H, ed ? t.editBg : t.bg);

    // czas i ikona stanu po prawej
    uint32_t sec = s.ms / 1000;
    char time[12];
    snprintf(time, sizeof(time), "%02u:%02u", unsigned(sec / 60), unsigned(sec % 60));
    spr_.setTextDatum(TR_DATUM);
    spr_.setTextColor(t.text);
    spr_.drawString(time, W - 6, 4);
    int ix = W - 6 - spr_.textWidth(time) - 16;
    if (s.state == PlayerState::Playing) spr_.fillTriangle(ix, 7, ix, 17, ix + 9, 12, t.playing);
    else if (s.state == PlayerState::Paused) { spr_.fillRect(ix, 7, 3, 11, t.text); spr_.fillRect(ix + 6, 7, 3, 11, t.text); }
    else spr_.fillRect(ix, 8, 9, 9, t.dim);

    spr_.setTextDatum(TL_DATUM);
    if (!s.loaded) {
        spr_.setTextColor(t.dim);
        spr_.drawString(fit("Brak utworu", ix - 12), 6, 4);
    } else {
        spr_.setTextColor(t.text);
        spr_.drawString(fit(ascii(s.title), ix - 12), 6, 4);
        String rel = fit(ascii(s.released), 120);
        spr_.setTextColor(t.dim);
        spr_.setTextDatum(TR_DATUM);
        spr_.drawString(rel, W - 6, 22);
        spr_.setTextDatum(TL_DATUM);
        spr_.drawString(fit(ascii(s.author), W - 20 - spr_.textWidth(rel)), 6, 22);
    }

    // trzecia linia: szczegoly albo podpowiedz
    spr_.setTextFont(1);
    spr_.setTextColor(t.dim);
    if (s.loaded) {
        String info = "Utwor " + String(s.song) + "/" + String(s.songs) + "   " +
                      (s.sidModel ? "8580" : "6581") + "   " + (s.ntsc ? "NTSC" : "PAL") + (s.rsid ? "   RSID" : "");
        spr_.drawString(info, 6, 44);
    }
    String right;
    if (ed) right = "krec: podutwor";
    else if (field_ == Field::Now) right = input_.encoderOnly() ? "Wybierz: pauza" : "Wybierz: podutwor";
    else right = "CPU " + String(s.cpuLoad) + "%";
    spr_.setTextDatum(TR_DATUM);
    spr_.setTextColor(field_ == Field::Now ? t.accent : t.dim);
    spr_.drawString(right, W - 6, 44);
    pushField(Field::Now, NOW_Y, NOW_H);
}

void PlayerLcd::drawVolume() {
    const LcdTheme& t = theme();
    bool ed = edit_ && field_ == Field::Volume;
    band(VOL_H, ed ? t.editBg : t.bg);
    spr_.setTextColor(t.text);
    spr_.drawString("Glosnosc", 6, 3);
    const int bx = 92, by = 7, bw = 170, bh = 9;
    int vol = last_.volume;
    spr_.fillRect(bx, by, bw, bh, t.barBg);
    spr_.fillRect(bx, by, bw * vol / 100, bh, last_.muted ? t.dim : t.accent);
    spr_.setTextDatum(TR_DATUM);
    spr_.drawString(last_.muted ? String("MUTE") : String(String(vol) + "%"), W - 6, 3);
    pushField(Field::Volume, VOL_Y, VOL_H);
}

void PlayerLcd::drawSource() {
    const LcdTheme& t = theme();
    bool ed = edit_ && field_ == Field::Source;
    band(SRC_H, ed ? t.editBg : t.bg);
    spr_.setTextColor(t.text);
    spr_.drawString("Zrodlo", 6, 3);
    const SourceInfo& info = ed ? PLAYER_SOURCES[srcPick_] : player_.sourceInfo();
    bool ready = srcReady_;
    String err = srcError_;
    if (ed) {   // podglad wybieranego zrodla
        SidSource* src = sourceImpl(info.type);
        ready = src && src->ready();
        err = src ? src->error() : "";
    }
    String state = ready ? String("gotowe") : fit(ascii(err), 110);
    spr_.setTextDatum(TR_DATUM);
    spr_.setTextColor(ready ? t.dim : t.error);
    spr_.drawString(state, W - 6, 3);
    String name = ascii(info.name);
    if (ed) name = "< " + name + " >";
    spr_.setTextDatum(TL_DATUM);
    spr_.setTextColor(ed ? t.accent : t.text);
    spr_.drawString(fit(name, W - 92 - spr_.textWidth(state) - 12), 80, 3);
    pushField(Field::Source, SRC_Y, SRC_H);
}

void PlayerLcd::drawPath() {
    const LcdTheme& t = theme();
    band(PATH_H, t.bg);
    spr_.drawFastHLine(0, PATH_H - 1, W, t.barBg);
    if (msgUntil_) {
        spr_.setTextColor(t.error);
        spr_.drawString(fit(ascii(msg_), W - 12), 6, 0);
    } else {
        int n = rowCount();
        String cnt = n ? String(String(cursor_ + 1) + "/" + String(n)) : String("pusto");
        if (player_.filesTruncated()) cnt += "+";
        spr_.setTextDatum(TR_DATUM);
        spr_.setTextColor(t.dim);
        spr_.drawString(cnt, W - 6, 0);
        spr_.setTextDatum(TL_DATUM);
        spr_.setTextColor(t.text);
        spr_.drawString(fit(ascii(player_.dir()), W - 24 - spr_.textWidth(cnt), true), 6, 0);
    }
    spr_.pushSprite(0, PATH_Y, 0, 0, W, PATH_H);
}

void PlayerLcd::drawRow(int slot) {
    const LcdTheme& t = theme();
    int row = top_ + slot;
    bool sel = row == cursor_ && field_ == Field::List;
    uint16_t bg = sel ? t.selBg : t.bg;
    band(ROW_H, bg);
    if (row < rowCount()) {
        const PlayerFile* e = rowEntry(row);
        bool playing = e && !e->dir && row == playRow_;
        String name = !e ? String("..") : e->dir ? String("[" + ascii(e->name) + "]") : ascii(e->name);
        uint16_t col = (!e || e->dir) ? t.folder : playing ? t.playing : t.text;
        if (sel) col = t.selText;
        String size;
        if (e && !e->dir) size = String(e->size / 1024.0f, 1) + "K";
        int avail = W - NAME_X - 6;
        int sizeW = size.length() ? spr_.textWidth(size) + 8 : 0;
        int w = spr_.textWidth(name);
        if (sel) {
            if (w > avail - sizeW) sizeW = 0;   // podswietlona dluga nazwa: bez rozmiaru pliku
            scrollMax_ = w > avail ? w - avail : 0;
        }
        if (sel && scrollMax_ > 0) {
            // nadal za dluga: przewijanie w poziomie
            int off = scrollPos_ < scrollMax_ ? scrollPos_ : scrollMax_;
            spr_.setTextColor(col);
            spr_.drawString(name, NAME_X - off, 1);
            spr_.fillRect(0, 0, NAME_X, ROW_H, bg);
            spr_.fillRect(W - 6, 0, 6, ROW_H, bg);
        } else {
            spr_.setTextColor(col);
            spr_.drawString(fit(name, avail - sizeW), NAME_X, 1);
            if (sizeW) {
                spr_.setTextDatum(TR_DATUM);
                spr_.setTextColor(sel ? t.selText : t.dim);
                spr_.drawString(size, W - 6, 1);
            }
        }
        if (playing) {
            spr_.setTextDatum(TL_DATUM);
            spr_.setTextColor(sel ? t.selText : t.playing);
            spr_.drawString(">", 4, 1);
        }
    }
    spr_.pushSprite(0, LIST_Y + slot * ROW_H, 0, 0, W, ROW_H);
}

String PlayerLcd::fit(const String& s, int maxW, bool keepEnd) {
    if (maxW <= 0) return "";
    if (spr_.textWidth(s) <= maxW) return s;
    String cut = s;
    while (cut.length()) {
        if (keepEnd) cut.remove(0, 1);
        else cut.remove(cut.length() - 1);
        String t = keepEnd ? String("..." + cut) : String(cut + "...");
        if (spr_.textWidth(t) <= maxW) return t;
    }
    return "";
}

/**
 * @brief Maps a Unicode code point (Latin-1 or Polish letters) to one ASCII character.
 * @param cp Code point.
 * @return ASCII character, '?' when unknown.
 */
static char foldChar(uint32_t cp) {
    static const char LATIN1[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTs" "aaaaaaaceeeeiiiidnooooo/ouuuuyty";
    if (cp >= 0xC0 && cp <= 0xFF) return LATIN1[cp - 0xC0];
    switch (cp) {
    case 0xA0: return ' ';
    case 0x104: return 'A'; case 0x105: return 'a';
    case 0x106: return 'C'; case 0x107: return 'c';
    case 0x118: return 'E'; case 0x119: return 'e';
    case 0x141: return 'L'; case 0x142: return 'l';
    case 0x143: return 'N'; case 0x144: return 'n';
    case 0x15A: return 'S'; case 0x15B: return 's';
    case 0x179: return 'Z'; case 0x17A: return 'z';
    case 0x17B: return 'Z'; case 0x17C: return 'z';
    default: return '?';
    }
}

String PlayerLcd::ascii(const String& s) {
    String out;
    out.reserve(s.length());
    size_t n = s.length();
    for (size_t i = 0; i < n; i++) {
        uint8_t c = uint8_t(s[i]);
        if (c < 0x80) { out += c < 0x20 ? ' ' : char(c); continue; }
        auto cont = [&](size_t k) { return k < n && (uint8_t(s[k]) & 0xC0) == 0x80; };
        uint32_t cp = c;   // bajt spoza UTF-8 = ISO-8859-1 (napisy z plikow SID)
        if ((c & 0xE0) == 0xC0 && cont(i + 1)) {
            cp = ((c & 0x1F) << 6) | (uint8_t(s[i + 1]) & 0x3F);
            i += 1;
        } else if ((c & 0xF0) == 0xE0 && cont(i + 1) && cont(i + 2)) {
            cp = ((c & 0x0F) << 12) | ((uint8_t(s[i + 1]) & 0x3F) << 6) | (uint8_t(s[i + 2]) & 0x3F);
            i += 2;
        }
        out += foldChar(cp);
    }
    return out;
}
#endif
