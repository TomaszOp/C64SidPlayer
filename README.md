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
| `lib/Player/` | **biblioteka odtwarzacza**: wyjście audio I2S, źródła plików (`SidSource.h`: flash, karta SD, DeepSID) z folderami, sterowanie |
| `lib/NvsSettings/` | wspólny mechanizm ustawień w pamięci NVS (lista pól w pliku `.def`, zapis tylko zmian) |
| `lib/PlayerMemory/` | ustawienia odtwarzacza (klasa `PlayerMemory`, lista w `PlayerSettings.def`) |
| `lib/PlayerWeb/` | przykład: panel WWW dla telefonu i PC (klasa `PlayerWeb`, skórki: `PlayerWebSkins.h`, strony `PlayerWebPage*.h`) |
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
    player.setSource(SourceType::SdCard);   // źródło plików (domyślnie pamięć flash)
    player.cd("Hubbard");                   // folder; ".." = poziom wyżej
    player.play("Commando.sid");            // albo playIndex(1) albo "/Hubbard/Commando.sid"
}
```

| Grupa | Metody |
|---|---|
| odtwarzanie | `play(nazwa, utwór)`, `playIndex(n)`, `nextFile()`, `prevFile()`, `stop()`, `pause()`, `resume()`, `togglePause()` |
| podutwory | `setSong(n)`, `nextSong()`, `prevSong()`, `restartSong()` |
| dźwięk | `setVolume(0-100)`, `volumeUp()`, `volumeDown()`, `setMute()`, `toggleMute()`, `setModel(-1/0/1)`, `setTestTone()` |
| źródła | `setSource(typ)`, `source()`, `sourceInfo()`, `sourceReady()`, `sourceError()`, `activeSource()` |
| foldery | `cd(folder)`, `dir()`, `dir(typ)`, `setDir(typ, folder)` – każde źródło pamięta swój folder |
| pliki | `files()` (foldery + pliki), `fileCount()`, `fileName(n)`, `removeFile(nazwa)`, `freeSpace()`, `uploadAllowed()`, `prepareUpload()`, `sanitizeName()` |
| stan | `status()` (tytuł, autor, utwór, czas, głośność, obciążenie CPU...), `state()`, `isPlaying()`, `audioError()` |

Metody zwracające `String` zwracają pusty napis przy powodzeniu, a w razie błędu jego opis.
Pauza zatrzymuje muzykę w miejscu; wyciszenie tylko ścisza, a muzyka gra dalej.

### Zapamiętane ustawienia (PlayerMemory)

Głośność, wyciszenie, model SID, ostatni plik i podutwór oraz „graj po włączeniu”
są zapisywane w pamięci NVS ESP32. Przetrwają wyłączenie zasilania i wgranie
nowych plików przez „Upload Filesystem Image”. `main.cpp` co 5 s sprawdza, czy coś się
zmieniło, i zapisuje tylko zmienione wartości, więc flash się nie zużywa.

```cpp
PlayerMemory mem;
mem.load();            // brak zapisu = wartości domyślne
mem.apply(player);     // głośność, wyciszenie, model SID -> odtwarzacz
mem.capture(player);   // stan odtwarzacza -> ustawienia
mem.save();            // zapisz tylko zmiany
mem.setDefault(true);  // wartości domyślne i wyczyszczenie NVS
```

Każdy moduł ma własne ustawienia we własnej przestrzeni NVS: odtwarzacz w
`lib/PlayerMemory/PlayerSettings.def`, panel WWW w `lib/PlayerWeb/WebSettings.def`
(np. wybrana skórka; `PlayerWeb` sam je wczytuje i zapisuje). Wspólny mechanizm jest
w `lib/NvsSettings` – opis, jak dodać własny zestaw ustawień, jest w `NvsSettings.h`.

Nowe ustawienie odtwarzacza to jedna linia w `lib/PlayerMemory/PlayerSettings.def`, np.
`SETTING(uint8_t, brightness, "bright", 50)`. Od razu jest dostępne jako `mem.brightness`,
a odczyt, zapis, przywracanie wartości domyślnej i wypisywanie przez `mem` działają same.
Gdy zmienisz typ istniejącego ustawienia, zwiększ wersję
(`NVS_VERSION`) w `PlayerMemory.h`, a stary zapis zostanie pominięty.

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

### Źródła plików i foldery

Pliki `.sid` mogą być w **pamięci flash** (LittleFS, zawsze dostępna) albo na **karcie SD**
(włączana flagą `-DPLAYER_SOURCE_SD=1` w `platformio.ini`; w środowisku S3 jest włączona).
Oba źródła obsługują foldery – np. całą kolekcję HVSC wystarczy skopiować na kartę.
Trzecie źródło, **DeepSid**, pobiera kolekcję HVSC z portalu https://deepsid.chordian.net
przez internet (flaga `-DPLAYER_SOURCE_DEEPSID=1`, opis niżej).

- Na stronie wybierasz źródło z listy „Źródło”, foldery są oznaczone ikoną 📁, „..” to poziom wyżej.
- Następny / poprzedni plik działa tylko w bieżącym folderze.
- Wgrywany plik trafia do bieżącego folderu, jeśli źródło na to pozwala.
- Zapamiętywane są: aktywne źródło oraz folder, ostatni plik i podutwór dla każdego źródła.
- Duże foldery są wysyłane na stronę w porcjach po 100 („Pokaż więcej”), najwyżej 1000 pozycji w folderze.
- Na flash cała ścieżka może mieć najwyżej ok. 63 znaki. Foldery na flash tworzysz w katalogu `data/` przed `uploadfs`.

Co wolno w danym źródle, ustawiasz w tablicy `PLAYER_SOURCES` w `lib/Player/PlayerSources.cpp`:

```cpp
//  typ                  id       opis            upload
{SourceType::Flash,  "flash", "Pamiec flash", true},
{SourceType::SdCard, "sd",    "Karta SD",     true},
{SourceType::DeepSid, "deepsid", "DeepSid",   false},
```

**DeepSid (internet)** – włączane flagą `-DPLAYER_SOURCE_DEEPSID=1` w `platformio.ini`
(w środowisku S3 jest włączone). Wymaga połączenia z WiFi (w trybie punktu dostępowego
źródło jest niedostępne).

- Lista folderu pochodzi z publicznego REST API DeepSID (`/api/v1/?folder=…` i `?file=…`),
  pokazywane są tylko podfoldery i pliki `.sid`. Plik jest pobierany przy odtwarzaniu, jak na stronie DeepSID.
- Ścieżki są takie jak w HVSC, np. `/MUSICIANS/H/Hubbard_Rob/Commando.sid`.
- Folder, ostatni plik i podutwór są zapamiętywane tak samo jak dla flash i karty SD.
- Źródło jest tylko do odczytu: bez wgrywania i usuwania plików.
- Folder `/MUSICIANS` wczytuje się kilka–kilkanaście sekund (API zwraca wszystkie jego podfoldery).
- Połączenie HTTPS jest bez sprawdzania certyfikatu (pliki są publiczne).

Nowe źródło: wartość w `enum SourceType`, klasa pochodna `SidSource`,
pozycja w `PLAYER_SOURCES` i `sourceImpl()`, pola w `PlayerSettings.def` – opis w `SidSource.h`.

**Karta SD (moduł SPI)** – piny w `include/config.h` (`SD_PIN_*`):

| Moduł SD | ESP32-S3 | ESP32 |
|---|---|---|
| CS | GPIO10 | GPIO5 |
| MOSI | GPIO11 | GPIO23 |
| SCK / CLK | GPIO12 | GPIO18 |
| MISO | GPIO13 | GPIO19 |
| VCC / GND | 3V3 (5V, jeśli moduł ma stabilizator) / GND | jw. |

Karta musi być sformatowana w FAT32. Jeśli karta nie działa, zmniejsz `SD_SPI_FREQ` w `config.h`.

### Wygląd strony (skórki)

Na stronie, na liście „Wygląd”, wybierasz skórkę: **Nowoczesny** (domyślny) albo **Commodore 64**
(ekran C64, czcionka terminalowa, lista plików jak `LOAD"$",8`). Wybór jest zapamiętany w NVS.
Jeśli na komputerze lub telefonie zainstalujesz czcionkę „C64 Pro Mono”, skórka C64 użyje jej automatycznie.

Nowa skórka: skopiuj `PlayerWebPageC64.h` pod nową nazwą i zmień nazwę tablicy, potem
dopisz wartość do `enum WebSkin` w `PlayerWebSkins.h` i linię do `WEB_SKINS` w `PlayerWebSkins.cpp`.

### Komendy przez port szeregowy (115200)

```
ls | list                 folder: [podfoldery] i pliki (* = bieżący)
cd Hubbard | cd .. | cd / zmień folder        pwd   bieżący folder
src | src sd | src deepsid  źródła plików / zmień źródło
play 2 | play Commando.sid 3   graj plik (i podutwór);  play  wznów
p | pause                 pauza / wznów        stop     zatrzymaj
n | next,  b | prev       następny / poprzedni podutwór    song 4
> | nfile, < | pfile      następny / poprzedni plik w folderze
vol 70 | + | -            głośność             m | mute  wycisz / włącz
model 8580                wymuś model SID (auto / 6581 / 8580)
del 3                     usuń plik
info | ip | tone | ?      stan / adres IP / ton testowy / IP + pomoc
mem | mem save | mem reset  zapamiętane ustawienia / zapisz teraz / domyślne
auto on | off             graj ostatni plik po włączeniu zasilania
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
