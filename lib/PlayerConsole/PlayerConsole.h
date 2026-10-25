// PlayerConsole - sterowanie biblioteka Player komendami tekstowymi
// (port szeregowy / COM, albo dowolny inny Stream). Przyklad uzycia biblioteki.
//
//   PlayerConsole console(player, Serial);
//   console.start();      // w setup()
//   console.handle();     // w loop()
//
// Wpisz "help" albo "?", aby zobaczyc liste komend.
#pragma once
#include <Arduino.h>
#include "Player.h"

class PlayerConsole {
public:
    PlayerConsole(Player& player, Stream& io);

    void start();
    void stop();
    void handle();                  // wywoluj czesto w loop()
    bool running() const { return running_; }

    void printStatus();
    void printHelp();
    void printIp();

private:
    void execute(const String& line);
    void report(const String& error, bool showStatus = true);
    String fileArg(const String& arg);

    Player& player_;
    Stream& io_;
    bool running_;
    String line_;
};
