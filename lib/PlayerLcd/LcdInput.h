/**
 * @file LcdInput.h
 * @brief LcdInput - rotary encoder (hardware PCNT counter) and push buttons turned into UI events.
 *
 * Two variants, chosen by the pins:
 *   - encoder + two buttons: Back (button 1), Select (button 2), encoder push = pause / stop,
 *   - encoder only (pinBack and pinSelect < 0): encoder push short = Select, long = Back,
 *     very long = Home.
 * The class knows nothing about the screen or the player.
 */
#pragma once
#if USE_LCD
#include <Arduino.h>

/** @brief UI event produced by LcdInput::poll(). */
enum class LcdEvent : uint8_t {
    None,       ///< Nothing happened.
    Turn,       ///< Encoder turned; the number of detents is in LcdInput::turnSteps().
    Select,     ///< "Select" (button 2, or a short encoder push in the encoder-only variant).
    Back,       ///< "Back" (button 1, or a long encoder push in the encoder-only variant).
    Home,       ///< Go back to the "Now playing" field (long button 1 / very long encoder push).
    PlayPause,  ///< Pause / resume (short encoder push, only with two buttons).
    Stop,       ///< Stop (long encoder push, only with two buttons).
    HoldHint,   ///< Encoder-only variant: a long press passed a threshold (LcdInput::holdLevel()).
};

/** @brief Pins and options of the controls. A pin < 0 = not connected. */
struct LcdInputConfig {
    int pinA = 40;              ///< Encoder A (CLK).
    int pinB = 41;              ///< Encoder B (DT).
    int pinPush = 42;           ///< Encoder push button.
    int pinBack = 1;            ///< Button 1 "Back" (-1 = encoder-only variant).
    int pinSelect = 39;         ///< Button 2 "Select" (-1 = encoder-only variant).
    uint8_t countsPerDetent = 4;   ///< PCNT counts per encoder click (4 for most encoders, 2 for some).
    bool reverse = false;       ///< Swap the turning direction.
    uint16_t longMs = 600;      ///< Long press threshold in ms.
    uint16_t veryLongMs = 2000; ///< Very long press threshold in ms (encoder-only variant).
};

/**
 * @brief Encoder and buttons reader; call poll() often from loop().
 */
class LcdInput {
public:
    /**
     * @brief Configures the pins and the PCNT unit.
     * @param cfg Pins and options (copied).
     */
    void begin(const LcdInputConfig& cfg);
    /**
     * @brief Returns the next event (one per call; call until None).
     * @return Event.
     */
    LcdEvent poll();
    /** @brief Detents of the last Turn event (negative = counter-clockwise). @return Steps. */
    int turnSteps() const { return turn_; }
    /** @brief Whether the last Turn event was fast (for acceleration). @return true when fast. */
    bool turnFast() const { return fast_; }
    /** @brief Threshold reached by the held encoder button (HoldHint): 1 = long, 2 = very long, 0 = none. */
    uint8_t holdLevel() const { return push_.level; }
    /** @brief Whether only the encoder is used (no separate buttons). @return true in the encoder-only variant. */
    bool encoderOnly() const { return cfg_.pinBack < 0 && cfg_.pinSelect < 0; }
    /** @brief millis() of the last input activity. @return Time in ms. */
    uint32_t lastActivity() const { return lastActivity_; }

private:
    /** @brief Debounced button state. */
    struct Button {
        int pin = -1;           ///< GPIO, -1 = none.
        bool down = false;      ///< Debounced state: pressed.
        bool raw = false;       ///< Last raw reading.
        uint32_t rawSince = 0;  ///< millis() when the raw reading changed.
        uint32_t downSince = 0; ///< millis() of the press.
        uint8_t level = 0;      ///< Long-press thresholds already reported (0, 1, 2).
    };
    /**
     * @brief Reads and debounces one button.
     * @param b Button.
     * @return 1 = pressed now, -1 = released now, 0 = no change.
     */
    int8_t update(Button& b);
    /** @brief Reads the PCNT counter. @return Whole detents since the previous call. */
    int readDetents();

    LcdInputConfig cfg_;       ///< Configuration.
    Button push_, back_, select_;   ///< Encoder push and the two buttons.
    int16_t lastRaw_ = 0;      ///< Previous PCNT reading.
    int acc_ = 0;              ///< Counts not yet forming a whole detent.
    int turn_ = 0;             ///< Detents of the last Turn event.
    bool fast_ = false;        ///< Last Turn event was fast.
    uint32_t lastTurn_ = 0;    ///< millis() of the previous Turn event.
    uint32_t lastActivity_ = 0;   ///< millis() of the last activity.
    bool ready_ = false;       ///< PCNT configured.
};
#endif
