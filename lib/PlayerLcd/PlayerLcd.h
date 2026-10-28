/**
 * @file PlayerLcd.h
 * @brief PlayerLcd - ILI9341 320x240 display (TFT_eSPI), rotary encoder and two buttons
 *        controlling the Player library. An example of using the library, like PlayerWeb.
 *
 * Enabled with -DUSE_LCD=1 (environment esp32s3-n16r8-lcd in platformio.ini); the display pins
 * are TFT_eSPI build flags (TFT_SCLK, TFT_MOSI, TFT_CS, TFT_DC, TFT_RST).
 *
 * Screen (top to bottom): Now playing, Volume, Source, folder path, list of folders and files.
 * The encoder moves the focus between the fields; Select edits a field, Back leaves the edit.
 * On the list the encoder moves the highlighted row at once: Select plays a file, enters a folder
 * or (on "..") goes to the parent folder; Back goes to the parent folder.
 * Without the two buttons (pins < 0) the encoder push does both: short = Select, long = Back.
 *
 * @code
 *   PlayerLcd lcd(player);
 *   lcd.begin();     // in setup(), after player.begin()
 *   lcd.handle();    // in loop(), from the same task that calls the Player file methods
 * @endcode
 * Description: docs/LCD.md
 */
#pragma once
#if USE_LCD
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Player.h"
#include "LcdInput.h"
#include "LcdSettings.h"
#include "LcdTheme.h"

/** @brief Display and controls configuration passed to PlayerLcd::begin(). */
struct LcdConfig {
    LcdInputConfig input;       ///< Encoder and buttons.
    int pinBacklight = 2;       ///< Backlight (PWM through a transistor), -1 = always on.
    uint8_t rotation = 1;       ///< TFT_eSPI rotation: 1 or 3 = landscape 320x240.
    uint8_t ledcChannel = 0;    ///< LEDC channel of the backlight (7 is the PHI2 clock of a real SID).
};

/**
 * @brief Player user interface on an ILI9341 LCD with an encoder and buttons.
 */
class PlayerLcd {
public:
    /**
     * @brief Creates the interface (nothing is started yet).
     * @param player Player to control.
     */
    explicit PlayerLcd(Player& player);
    /**
     * @brief Starts the display, the controls and reads the settings from NVS.
     * @param cfg Pins and options (copied).
     */
    void begin(const LcdConfig& cfg = LcdConfig());
    /** @brief Reads the controls, follows the player state and redraws what changed; call it often in loop(). */
    void handle();
    /**
     * @brief Shows a message in the path bar for a while.
     * @param text Message (ASCII; other characters are simplified).
     * @param ms   How long to show it.
     */
    void message(const String& text, uint32_t ms = 3000);
    /** @brief Settings remembered in NVS (change them, then call applySettings() and settings().save()). */
    LcdSettings& settings() { return settings_; }
    /** @brief Applies the theme and brightness from settings() and redraws the screen. */
    void applySettings();

private:
    /** @brief Fields of the main screen, in focus order. */
    enum class Field : uint8_t { Now, Volume, Source, List };
    /** @brief Parts of the screen to redraw. */
    enum Dirty : uint8_t { D_NOW = 1, D_VOL = 2, D_SRC = 4, D_PATH = 8, D_LIST = 16, D_ALL = 31 };

    // --- controls ---
    /** @brief Handles one input event. @param e Event. */
    void onEvent(LcdEvent e);
    /** @brief Encoder turned. @param steps Detents. @param fast Fast turn (acceleration). */
    void onTurn(int steps, bool fast);
    /** @brief "Select" pressed. */
    void onSelect();
    /** @brief "Back" pressed. */
    void onBack();
    /** @brief Pause / resume, with a message when nothing is loaded. */
    void togglePause();
    /** @brief Leaves the edit mode (the source choice is cancelled). */
    void endEdit();
    /** @brief Moves the focus to another field. @param f New field. */
    void focus(Field f);

    // --- list ---
    /** @brief Whether the list starts with "..". @return true outside the root folder. */
    bool hasUpRow();
    /** @brief Number of list rows (".." + folders + files). @return Rows. */
    int rowCount();
    /** @brief Entry of a list row. @param row Row. @return Entry, nullptr for "..". */
    const PlayerFile* rowEntry(int row);
    /** @brief Row of the file that is playing in the current folder. @return Row, -1 when none. */
    int playingRow();
    /** @brief Row of an entry by name. @param name Name. @return Row, -1 when none. */
    int findRow(const String& name);
    /** @brief Select on the highlighted row: play / enter folder / parent folder. */
    void openRow();
    /** @brief Goes to the parent folder and highlights the folder we came from. @return false in the root. */
    bool goUp();
    /**
     * @brief Changes the folder (with a "Loading..." note for slow sources).
     * @param to Argument of Player::cd().
     * @return "" or an error.
     */
    String changeDir(const String& to);
    /**
     * @brief Moves the cursor and marks the rows to redraw.
     * @param row New cursor row (clamped).
     */
    void setCursor(int row);
    /**
     * @brief Shows "Loading..." at once when a source is slow (DeepSid), before a blocking call.
     * @param type Source that will be read.
     */
    void loadingNote(SourceType type);

    // --- state ---
    /** @brief Reads Player::status() and the folder listing, marks changed parts. */
    void pollStatus();
    /** @brief Edit timeout and backlight dimming. */
    void idleTasks();
    /**
     * @brief Sets the backlight.
     * @param percent 0..100.
     */
    void backlight(uint8_t percent);

    // --- drawing ---
    /** @brief Redraws the dirty parts. */
    void draw();
    /** @brief Draws the "Now playing" field. */
    void drawNow();
    /** @brief Draws the volume field. */
    void drawVolume();
    /** @brief Draws the source field. */
    void drawSource();
    /** @brief Draws the path / message bar. */
    void drawPath();
    /**
     * @brief Draws one list slot.
     * @param slot Visible slot 0..LIST_ROWS-1.
     */
    void drawRow(int slot);
    /**
     * @brief Clears the sprite for a band of the given height.
     * @param h  Height in pixels.
     * @param bg Background colour.
     */
    void band(int h, uint16_t bg);
    /**
     * @brief Draws the focus frame of a field (when focused) and sends the band to the display.
     * @param f Field of the band.
     * @param y Top of the band on the display.
     * @param h Height.
     */
    void pushField(Field f, int y, int h);
    /**
     * @brief Shortens text to fit a width (with "..."), using the current sprite font.
     * @param s        Text.
     * @param maxW     Width in pixels.
     * @param keepEnd  true = cut the beginning (paths), false = cut the end.
     * @return Text that fits.
     */
    String fit(const String& s, int maxW, bool keepEnd = false);
    /**
     * @brief Converts ISO-8859-1 or UTF-8 text to ASCII (the built-in fonts have ASCII only).
     * @param s Text.
     * @return ASCII text ("a" for "ą", "o" for "ö"...).
     */
    static String ascii(const String& s);
    /** @brief Current theme. @return Theme colours. */
    const LcdTheme& theme() const;

    Player& player_;            ///< Controlled player.
    TFT_eSPI tft_;              ///< Display.
    TFT_eSprite spr_;           ///< Off-screen band (320x60) - drawing without flicker.
    bool sprOk_ = false;        ///< Sprite allocated.
    LcdInput input_;            ///< Encoder and buttons.
    LcdSettings settings_;      ///< NVS settings.
    LcdConfig cfg_;             ///< Configuration from begin().
    bool started_ = false;      ///< begin() was called.

    Field field_ = Field::List; ///< Focused field.
    bool edit_ = false;         ///< Focused field is being edited.
    uint8_t srcPick_ = 0;       ///< Source shown while editing (index into PLAYER_SOURCES).
    int cursor_ = 0;            ///< Highlighted list row.
    int top_ = 0;               ///< First visible list row.
    uint8_t dirty_ = D_ALL;     ///< Parts to redraw.
    int rowA_ = -1, rowB_ = -1; ///< Single rows to redraw (cursor moves), -1 = none.

    PlayerStatus last_;         ///< Status at the last poll.
    bool haveLast_ = false;     ///< last_ is valid.
    uint32_t listGen_ = 0;      ///< Player::listGeneration() of the shown list.
    uint32_t lastPoll_ = 0;     ///< millis() of the last pollStatus().
    uint32_t lastSrcCheck_ = 0; ///< millis() of the last source readiness check.
    bool srcReady_ = false;     ///< Active source ready (cached).
    String srcError_;           ///< Active source error (cached).

    String msg_;                ///< Message in the path bar.
    uint32_t msgUntil_ = 0;     ///< millis() when the message disappears.
    uint32_t cursorSince_ = 0;  ///< millis() of the last cursor move (long-name scrolling).
    int scrollMax_ = 0;         ///< Overflow of the highlighted name in pixels (0 = fits).
    int scrollPos_ = 0;         ///< Current scroll offset in pixels.
    uint32_t lastScroll_ = 0;   ///< millis() of the last scroll step.
    int playRow_ = -1;          ///< Row of the playing file, computed before drawing the list.
    uint8_t blNow_ = 255;       ///< Current backlight in % (255 = not set yet).
    bool asleep_ = false;       ///< Backlight dimmed / off - the next input only wakes the screen.
};
#endif
