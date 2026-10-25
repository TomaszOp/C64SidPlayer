#include "PlayerConsole.h"
#include <WiFi.h>

PlayerConsole::PlayerConsole(Player& player, Stream& io)
    : player_(player), io_(io), running_(false) {}

void PlayerConsole::start() {
    running_ = true;
    line_ = "";
    io_.println("Wpisz \"help\" albo \"?\", aby zobaczyc komendy.");
}

void PlayerConsole::stop() { running_ = false; }

void PlayerConsole::handle() {
    if (!running_) return;
    while (io_.available()) {
        char c = io_.read();
        if (c != '\n' && c != '\r') {
            if (line_.length() < 120) line_ += c;
            continue;
        }
        line_.trim();
        if (!line_.length()) continue;
        String cmd = line_;
        line_ = "";
        io_.println("> " + cmd);
        execute(cmd);
    }
}

void PlayerConsole::report(const String& error, bool showStatus) {
    if (error.length()) io_.println("blad: " + error);
    else if (showStatus) printStatus();
}

// "3" -> nazwa 3. pliku, inaczej nazwa podana wprost
String PlayerConsole::fileArg(const String& arg) {
    int n = arg.toInt();
    return n > 0 ? player_.fileName(n) : arg;
}

void PlayerConsole::execute(const String& line) {
    String cmd = line, arg;
    int sp = line.indexOf(' ');
    if (sp > 0) { cmd = line.substring(0, sp); arg = line.substring(sp + 1); arg.trim(); }
    cmd.toLowerCase();

    if (cmd == "list" || cmd == "ls") {
        std::vector<PlayerFile> list = player_.files();
        String current = player_.status().file;
        for (size_t i = 0; i < list.size(); i++)
            io_.printf("%c%2u. %s (%u B)\n", list[i].name == current ? '*' : ' ',
                       unsigned(i + 1), list[i].name.c_str(), unsigned(list[i].size));
        if (list.empty()) io_.println("brak plikow - wgraj je przez strone WWW");
        io_.printf("wolne miejsce: %u B\n", unsigned(player_.freeSpace()));
    } else if (cmd == "play") {
        if (!arg.length()) { player_.resume(); report(""); return; }
        int song = 0;
        int sp2 = arg.lastIndexOf(' ');
        if (sp2 > 0) { song = arg.substring(sp2 + 1).toInt(); arg = arg.substring(0, sp2); }
        report(player_.play(fileArg(arg), song));
    } else if (cmd == "pause" || cmd == "p") {
        player_.togglePause();
        io_.println(player_.state() == PlayerState::Paused ? "pauza" : "gra");
    } else if (cmd == "resume") {
        player_.resume();
        report("");
    } else if (cmd == "stop") {
        player_.stop();
        io_.println("stop");
    } else if (cmd == "next" || cmd == "n") {
        report(player_.nextSong());
    } else if (cmd == "prev" || cmd == "b") {
        report(player_.prevSong());
    } else if (cmd == "song") {
        report(player_.setSong(arg.toInt()));
    } else if (cmd == "nfile" || cmd == ">") {
        report(player_.nextFile());
    } else if (cmd == "pfile" || cmd == "<") {
        report(player_.prevFile());
    } else if (cmd == "vol" || cmd == "v") {
        if (arg == "+") player_.volumeUp();
        else if (arg == "-") player_.volumeDown();
        else if (arg.length()) player_.setVolume(arg.toInt());
        io_.printf("glosnosc %d%%\n", player_.volume());
    } else if (cmd == "+") {
        player_.volumeUp();
        io_.printf("glosnosc %d%%\n", player_.volume());
    } else if (cmd == "-") {
        player_.volumeDown();
        io_.printf("glosnosc %d%%\n", player_.volume());
    } else if (cmd == "mute" || cmd == "m") {
        if (arg == "on" || arg == "1") player_.setMute(true);
        else if (arg == "off" || arg == "0") player_.setMute(false);
        else player_.toggleMute();
        io_.println(player_.muted() ? "wyciszone" : "dzwiek wlaczony");
    } else if (cmd == "model") {
        report(player_.setModel(arg == "6581" ? 0 : arg == "8580" ? 1 : -1));
    } else if (cmd == "del" || cmd == "rm") {
        String name = fileArg(arg);
        io_.println(player_.removeFile(name) ? "usunieto " + name : "nie usunieto " + name);
    } else if (cmd == "tone") {
        player_.setTestTone(!player_.testTone());
        io_.println(player_.testTone() ? "ton testowy 440 Hz WLACZONY (wpisz tone, aby wylaczyc)"
                                       : "ton testowy wylaczony");
    } else if (cmd == "info" || cmd == "i") {
        printStatus();
    } else if (cmd == "ip") {
        printIp();
    } else if (cmd == "?") {
        printIp();
        printHelp();
    } else {
        printHelp();
    }
}

void PlayerConsole::printHelp() {
    io_.println(
        "komendy:\n"
        "  list                     lista plikow (* = biezacy)\n"
        "  play <nr|nazwa> [utwor]  graj plik     play     wznow\n"
        "  p | pause                pauza / wznow  stop      zatrzymaj\n"
        "  n | next, b | prev       nastepny / poprzedni podutwor   song <n>\n"
        "  > | nfile, < | pfile     nastepny / poprzedni plik\n"
        "  vol <0-100> | + | -      glosnosc       m | mute  wycisz / wlacz\n"
        "  model <auto|6581|8580>   model ukladu SID\n"
        "  del <nr|nazwa>           usun plik\n"
        "  info | ip | tone | ?");
}

void PlayerConsole::printIp() {
    bool ap = WiFi.getMode() == WIFI_AP;
    IPAddress ip = ap ? WiFi.softAPIP() : WiFi.localIP();
    const char* host = WiFi.getHostname();
    io_.printf("IP: %s (%s %s), http://%s.local/\n", ip.toString().c_str(),
               ap ? "punkt dostepowy" : "WiFi", ap ? WiFi.softAPSSID().c_str() : WiFi.SSID().c_str(),
               host ? host : "esp32");
}

void PlayerConsole::printStatus() {
    PlayerStatus s = player_.status();
    const PlayerConfig& c = player_.config();
    io_.printf("audio: %s (BCK %d, WS %d, DATA %d), glosnosc %d%%%s%s\n", player_.audioError(),
               c.pinBck, c.pinWs, c.pinData, s.volume, s.muted ? ", WYCISZONE" : "",
               s.testTone ? ", ton testowy" : "");
    io_.printf("silnik SID: %s, obciazenie CPU audio: %u%%\n", s.engine, s.cpuLoad);
    if (!s.loaded) { io_.println("nic nie jest wczytane"); return; }
    const char* st = s.state == PlayerState::Playing ? "PLAY" : s.state == PlayerState::Paused ? "PAUZA" : "STOP";
    io_.printf("%s %s | %s | %s | %s\n", st, s.file.c_str(), s.title.c_str(), s.author.c_str(), s.released.c_str());
    io_.printf("utwor %d/%d, %s %s, SID %s%s, %.1f Hz, %u:%02u\n", s.song, s.songs,
               s.rsid ? "RSID" : "PSID", s.ntsc ? "NTSC" : "PAL",
               (s.modelSetting >= 0 ? s.modelSetting : s.sidModel) ? "8580" : "6581",
               s.modelSetting < 0 ? "" : " (wymuszony)", s.frameHz, unsigned(s.ms / 60000), unsigned(s.ms / 1000 % 60));
}
