/**
 * @file PlayerConsole.h
 * @brief PlayerConsole - controlling the Player library with text commands
 *        (serial port / COM, or any other Stream). An example of using the library.
 *
 * @code
 *   PlayerConsole console(player, Serial);
 *   console.start();      // in setup()
 *   console.handle();     // in loop()
 * @endcode
 * Type "help" or "?" to see the list of commands.
 */
#pragma once
#include <Arduino.h>
#include "Player.h"

class PlayerMemory;

/**
 * @brief Text command console for a Player.
 */
class PlayerConsole {
public:
    /**
     * @brief Creates the console (stopped).
     * @param player Player to control.
     * @param io     Stream for commands and replies.
     */
    PlayerConsole(Player& player, Stream& io);

    /** @brief Starts accepting commands and prints a hint. */
    void start();
    /** @brief Stops accepting commands. */
    void stop();
    /** @brief Reads input and executes complete lines; call it often in loop(). */
    void handle();
    /** @brief Whether the console is active. @return true after start(). */
    bool running() const { return running_; }
    /**
     * @brief Optional: enables the "mem" and "auto" commands for the remembered settings.
     * @param memory Settings object (nullptr = commands disabled).
     */
    void setMemory(PlayerMemory* memory) { memory_ = memory; }

    /** @brief Prints audio, source and current tune status. */
    void printStatus();
    /** @brief Prints the list of commands. */
    void printHelp();
    /** @brief Prints the IP address, network and mDNS name. */
    void printIp();
    /** @brief Re-reads and prints the current folder listing. */
    void printList();
    /** @brief Prints the compiled-in file sources and their state. */
    void printSources();

private:
    /**
     * @brief Parses and executes one command line.
     * @param line Command with arguments.
     */
    void execute(const String& line);
    /**
     * @brief Handles the "mem" and "auto" commands (and saves the settings).
     * @param cmd "mem" or "auto".
     * @param arg Argument ("", "save", "reset", "on", "off"...).
     */
    void memoryCommand(const String& cmd, const String& arg);
    /**
     * @brief Prints an error, or the status when there is none.
     * @param error      Error text ("" = success).
     * @param showStatus Print the status on success.
     */
    void report(const String& error, bool showStatus = true);
    /**
     * @brief Resolves a file argument: "3" -> name of the 3rd file, otherwise the name as given.
     * @param arg Number or name.
     * @return File name.
     */
    String fileArg(const String& arg);

    Player& player_;     ///< Controlled player.
    Stream& io_;         ///< Command input / reply output.
    bool running_;       ///< Console active.
    String line_;        ///< Line being typed.
    PlayerMemory* memory_ = nullptr;   ///< Remembered settings, nullptr = "mem"/"auto" disabled.
};
