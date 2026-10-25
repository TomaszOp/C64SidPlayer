// Narzedzie na PC: renderuje plik .sid do .wav tym samym silnikiem co ESP32.
// Kompilacja:  g++ -O2 -DSID_ENGINE=1 -Ilib/SidEngine tools/sid2wav.cpp lib/SidEngine/*.cpp lib/SidEngine/resid/resid.cpp -o sid2wav
//              (-DSID_ENGINE=0 = wlasny silnik)
// Uzycie:      ./sid2wav utwor.sid wynik.wav [sekundy] [nr_utworu]
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "sidplayer.h"

static void put32(FILE* f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void put16(FILE* f, uint16_t v) { fwrite(&v, 2, 1, f); }

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "uzycie: %s plik.sid wynik.wav [sekundy] [utwor]\n", argv[0]); return 1; }
    int seconds = argc > 3 ? atoi(argv[3]) : 30;
    int song = argc > 4 ? atoi(argv[4]) : 0;
    FILE* in = fopen(argv[1], "rb");
    if (!in) { perror(argv[1]); return 1; }
    std::vector<uint8_t> data(70000);
    size_t len = fread(data.data(), 1, data.size(), in);
    fclose(in);

    const uint32_t rate = 44100;
    SidPlayer player;
    player.init(rate);
    if (!player.load(data.data(), len) || !player.startSong(song)) {
        fprintf(stderr, "blad: %s\n", player.error());
        return 1;
    }
    const SidInfo& i = player.info();
    printf("%s: %s / %s / %s\n", i.rsid ? "RSID" : "PSID", i.name, i.author, i.released);
    printf("silnik: %s\n", SID_ENGINE_NAME);
    printf("utwor %d/%d, load $%04X init $%04X play $%04X, %s, %s, %.2f Hz\n",
           player.currentSong(), i.songs, i.loadAddr, i.initAddr, i.playAddr,
           i.ntsc ? "NTSC" : "PAL", i.model == Sid::MOS8580 ? "8580" : "6581", player.frameRateHz());

    std::vector<int16_t> pcm(size_t(rate) * seconds);
    player.render(pcm.data(), pcm.size());

    FILE* out = fopen(argv[2], "wb");
    if (!out) { perror(argv[2]); return 1; }
    uint32_t bytes = uint32_t(pcm.size() * 2);
    fwrite("RIFF", 1, 4, out); put32(out, 36 + bytes); fwrite("WAVEfmt ", 1, 8, out);
    put32(out, 16); put16(out, 1); put16(out, 1); put32(out, rate); put32(out, rate * 2);
    put16(out, 2); put16(out, 16); fwrite("data", 1, 4, out); put32(out, bytes);
    fwrite(pcm.data(), 2, pcm.size(), out);
    fclose(out);
    return 0;
}
