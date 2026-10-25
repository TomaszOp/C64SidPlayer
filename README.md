# C64SidPlayer – odtwarzacz muzyki SID z C64 dla ESP32

Odtwarzacz muzyki z Commodore 64 (pliki `.sid`, formaty PSID i RSID) dla ESP32.
Program emuluje procesor 6510 i układ dźwiękowy SID (6581 / 8580), więc muzyka
jest odtwarzana tak, jak robił to oryginalny C64: uruchamiany jest prawdziwy kod
odtwarzacza z pliku, a SID syntetyzuje dźwięk.

## Budowa projektu

Odtwarzacz jest biblioteką (`lib/Player`), a panel WWW i konsola COM to osobne
klasy, które tylko jej używają. `src/main.cpp` jest krótkim przykładem łączącym
wszystko; panel albo konsolę można wyłączyć (`USE_WEB`, `USE_CONSOLE`).

| Katalog / plik | Co robi |
|---|---|
| `lib/Player/` | **biblioteka odtwarzacza**: wyjście audio I2S, pliki `.sid` z pamięci flash (LittleFS), sterowanie |
| `lib/PlayerWeb/` | przykład: panel WWW dla telefonu i PC (klasa `PlayerWeb`, strona w `PlayerWebPage.h`) |
| `lib/PlayerConsole/` | przykład: komendy przez port szeregowy (klasa `PlayerConsole`, `DualSerial` dla USB+COM) |
| `lib/SidEngine/` | emulacja C64: CPU 6510 (`cpu6502.*`), ładowanie PSID/RSID (`sidplayer.*`), układ SID (własny `sid.*` lub `resid/`) |
| `src/main.cpp` | przykład: WiFi + Player + PlayerWeb + PlayerConsole |
| `include/config.h` | WiFi, piny, częstotliwość próbkowania, głośność |
| `tools/sid2wav.cpp` | ten sam silnik na PC: zamienia `.sid` na `.wav` (do testów) |
| `data/` | pliki `.sid` wgrywane do pamięci flash przez `pio run -t uploadfs` |

### Biblioteka Player w skrócie

```cpp
#include <Player.h>
Player player;

void setup() {
    PlayerConfig cfg;                 // piny I2S, częstotliwość, głośność...
    cfg.pinBck = 4; cfg.pinWs = 5; cfg.pinData = 6;
    player.begin(cfg);
    player.play("Commando.sid");      // albo player.playIndex(1)
}
```

| Grupa | Metody |
|---|---|
| odtwarzanie | `play(nazwa, utwór)`, `playIndex(n)`, `nextFile()`, `prevFile()`, `stop()`, `pause()`, `resume()`, `togglePause()` |
| podutwory | `setSong(n)`, `nextSong()`, `prevSong()`, `restartSong()` |
| dźwięk | `setVolume(0-100)`, `volumeUp()`, `volumeDown()`, `setMute()`, `toggleMute()`, `setModel(-1/0/1)`, `setTestTone()` |
| pliki | `files()`, `fileCount()`, `fileName(n)`, `removeFile(nazwa)`, `freeSpace()`, `fs()`, `sanitizeName()` |
| stan | `status()` (tytuł, autor, utwór, czas, głośność, obciążenie CPU...), `state()`, `isPlaying()`, `audioError()` |

Metody zwracające `String` zwracają pusty napis przy powodzeniu, a w razie błędu jego opis.
Pauza zatrzymuje muzykę w miejscu; wyciszenie tylko ścisza, a muzyka gra dalej.

## Silnik emulacji SID

Do wyboru są dwa silniki, ustawiane w `platformio.ini` dyrektywą preprocesora
w `build_flags` danego środowiska:

| Flaga | Silnik |
|---|---|
| `-DSID_ENGINE=1` (domyślnie) | **reSID 0.16** (Dag Lem): emulacja cykl po cyklu, prawdziwe przebiegi łączone 6581/8580, wierny filtr |
| `-DSID_ENGINE=0` | własny, lekki silnik (`lib/SidEngine/sid.*`) |
| `-DSID_RESID_INTERPOLATE=1` | tylko z reSID: próbkowanie z interpolacją, lepsza jakość, ok. 10× większe obciążenie CPU |

Aktywny silnik i obciążenie CPU pokazuje komenda `info` oraz strona WWW.
reSID pochodzi z projektu Teensy-reSID (Frank Bösing), z przywróconą obsługą
modelu 8580; pliki są w `lib/SidEngine/resid/`.

**Licencja:** reSID jest na licencji GNU GPL (v2+, adaptacja Teensy: v3+), patrz
`lib/SidEngine/resid/COPYING`. Program zbudowany z reSID podlega GPL: do użytku
własnego bez ograniczeń, a przy rozpowszechnianiu trzeba udostępnić kod źródłowy.

## Podłączenie dźwięku

**Zalecane: zewnętrzny DAC I2S** (PCM5102A, UDA1334A albo wzmacniacz MAX98357A z głośnikiem):

| DAC | ESP32 |
|---|---|
| BCK / BCLK | GPIO26 |
| LRCK / WS / LRC | GPIO25 |
| DIN / DATA | GPIO22 |
| VCC / GND | 3V3 (lub 5V dla MAX98357A) / GND |

PCM5102A: piny SCK→GND, FMT→GND, XSMT→3V3.

**ESP32-S3 (N16R8)**: środowisko `esp32s3-n16r8`, DAC I2S na innych pinach:
BCK → GPIO4, LRCK/WS → GPIO5, DIN → GPIO6. S3 nie ma wbudowanego DAC.
Kompilacja i wgranie: `pio run -e esp32s3-n16r8 -t upload` (oraz `-t uploadfs`).
Konsola działa na obu gniazdach płytki: „USB” i „COM”.

**Najprościej: wbudowany DAC** (8 bitów, gorsza jakość). W `platformio.ini` odkomentuj
`-DSID_OUTPUT_INTERNAL_DAC`. Dźwięk wychodzi na GPIO25 (oraz GPIO26):
GPIO25 → kondensator 10 µF → wejście wzmacniacza / słuchawki aktywne, GND → GND.

## Uruchomienie

1. Skopiuj `include/secrets.example.h` jako `include/secrets.h` i wpisz swoją sieć WiFi (albo zostaw puste: ESP32 utworzy sieć
   `ESP32-SID`, hasło `commodore64`, adres `http://192.168.4.1/`).
2. Wgraj program: `pio run -t upload`, potem `pio device monitor`.
3. Opcjonalnie wgraj pliki z katalogu `data/`: `pio run -t uploadfs`.
4. Otwórz stronę `http://c64sid.local/` (albo adres IP z monitora), wybierz plik `.sid`
   i kliknij „wyślij”. Utwór od razu zacznie grać.

Pliki SID: High Voltage SID Collection, https://www.hvsc.c64.org/

### Komendy przez port szeregowy (115200)

```
list                      lista plików (* = bieżący)
play 2 | play Commando.sid 3   graj plik (i podutwór);  play  wznów
p | pause                 pauza / wznów        stop     zatrzymaj
n | next,  b | prev       następny / poprzedni podutwór    song 4
> | nfile, < | pfile      następny / poprzedni plik
vol 70 | + | -            głośność             m | mute  wycisz / włącz
model 8580                wymuś model SID (auto / 6581 / 8580)
del 3                     usuń plik
info | ip | tone | ?      stan / adres IP / ton testowy / IP + pomoc
```

## Test na PC

```
g++ -O2 -DSID_ENGINE=1 -Ilib/SidEngine tools/sid2wav.cpp lib/SidEngine/*.cpp lib/SidEngine/resid/resid.cpp -o sid2wav
./sid2wav utwor.sid wynik.wav 60 1
```

## Ograniczenia

- Obsługiwany jest jeden SID na `$D400` (bez plików 2SID/3SID).
- Procedura `play` jest wywoływana raz na ramkę (50 Hz PAL, 60 Hz NTSC lub częstotliwość
  timera CIA, także multispeed). Utwory z samplami („digi”) odtwarzanymi przez NMI
  oraz RSID wymagające dokładnej emulacji VIC/CIA mogą grać niepoprawnie.
- W silniku własnym filtr i przebiegi łączone to przybliżenie charakterystyki prawdziwych układów.
- Jeśli ESP32 nie nadąża (trzaski), zmniejsz nadpróbkowanie: `-DSID_OVERSAMPLE=2`
  albo `SAMPLE_RATE` do 32000.
