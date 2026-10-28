# LCD ILI9341 + rotary encoder + buttons

Optional local control panel for the ESP32-S3 N16R8: a 2.8" ILI9341 320×240 SPI display
(TFT_eSPI library), a rotary encoder with a push button and two buttons (**1 = Back**,
**2 = Select**). It works next to the web panel and the serial console and shows the same player
state (one current folder, one volume).

Polish version below.

## Build

Environment `esp32s3-n16r8-lcd` in `platformio.ini` (extends `esp32s3-n16r8`):

```
pio run -e esp32s3-n16r8-lcd -t upload
```

| Flag | Meaning |
| --- | --- |
| `-DUSE_LCD=1` | compiles `lib/PlayerLcd` and starts it in `main.cpp` |
| `-DLCD_BUTTONS=1` | encoder + buttons Back and Select (default) |
| `-DLCD_BUTTONS=0` | encoder only: short push = Select, long = Back, very long = Now playing |
| `-DTFT_*`, `-DILI9341_DRIVER` ... | TFT_eSPI configuration (no editing of the library) |

Without `USE_LCD` nothing of the LCD is compiled and TFT_eSPI is not downloaded. The classic
ESP32 is not supported (`config.h` stops the build with `#error`).

## Wiring (ESP32-S3)

The display has its own SPI bus (HSPI), separate from the SD card (GPIO 10–13).

| Signal | GPIO | Where it is set |
| --- | --- | --- |
| TFT SCK | 14 | `platformio.ini` `-DTFT_SCLK` |
| TFT MOSI (SDI) | 21 | `-DTFT_MOSI` |
| TFT MISO (SDO) | – | not connected |
| TFT CS | 9 | `-DTFT_CS` |
| TFT DC (RS) | 47 | `-DTFT_DC` |
| TFT RESET | 3V3 or EN | `-DTFT_RST=-1` (software reset) |
| TFT LED (backlight) | 2 | `config.h` `LCD_PIN_BL`, PWM, LEDC channel 0 |
| Encoder A (CLK) | 40 | `LCD_PIN_ENC_A` |
| Encoder B (DT) | 41 | `LCD_PIN_ENC_B` |
| Encoder push | 42 | `LCD_PIN_PUSH` |
| Button 1 – Back | 1 | `LCD_PIN_BACK` |
| Button 2 – Select | 39 | `LCD_PIN_SELECT` |

- Display: VCC 3V3, GND. The backlight takes 50–80 mA: drive LED through a transistor
  (e.g. BC547 with 1 kΩ to the base, or AO3400). `LCD_PIN_BL -1` = LED wired to 3V3, no dimming.
- Bare encoder (no module): common pin C and one push-button pin to GND, A/B/push to the GPIOs.
  Internal pull-ups are used and the PCNT hardware filter removes contact bounce. If it skips
  steps, add 10 kΩ to 3V3 and 10 nF to GND on A and B.
- Buttons: between the GPIO and GND (internal pull-up), optionally 100 nF.
- If one click moves by two, set `LCD_ENC_COUNTS 2`; wrong direction: `LCD_ENC_REVERSE 1`.
  Red and blue swapped: uncomment `-DTFT_RGB_ORDER=TFT_BGR`.
- Free pins left: 38 and 48 (RGB LED on the DevKitC-1).

## Screen

```text
+--------------------------------------------------+
| Commando                              > 02:13    |  Now playing
| Rob Hubbard                     1985 Elite       |
| Utwor 1/3   8580   PAL              CPU 34%      |
+--------------------------------------------------+
| Glosnosc [##############------]  70%             |  Volume
+--------------------------------------------------+
| Zrodlo   Karta SD                      gotowe    |  Source
+--------------------------------------------------+
| /MUSICIANS/H/Hubbard_Rob               12/87     |  path / messages
|  ..                                              |
|  [Ace_II]                                        |  list: 6 rows
|> Commando.sid                          5.2K      |  > = playing
+--------------------------------------------------+
```

The focused field has a frame, a field being edited has a darker background, the highlighted
list row is filled. Errors and "Wczytywanie..." (loading) appear for a few seconds in the path bar.
Long names are cut with "..."; the highlighted name scrolls after 1 s. The built-in fonts are
ASCII only, so Polish and Latin-1 letters are shown without accents (ą → a, ö → o).

## Controls

| Where | Encoder | Select (2) | Back (1) |
| --- | --- | --- | --- |
| Moving between fields | next / previous field | edit the field | – |
| Now playing (edit) | previous / next subtune | end edit | end edit |
| Volume (edit) | louder / quieter, heard at once; fast turn = 5 | end edit | end edit |
| Source (edit) | choose a source (preview only) | confirm, load the list, go to the list | cancel |
| List | previous / next row; fast turn = 5 rows; up on the first row = Source field | file: play; folder: enter; `..`: parent | parent folder; in the root: leave the list |

| Extra | Encoder + 2 buttons | Encoder only (`LCD_BUTTONS=0`) |
| --- | --- | --- |
| Select | button 2 | short encoder push |
| Back | button 1 | long push (≥ 0.6 s), on release |
| Back to Now playing | long button 1 | very long push (≥ 2 s) |
| Pause / resume | short encoder push | Select on the Now playing field |
| Stop | long encoder push | web panel / console |
| Change subtune | edit Now playing | web panel / console |

- After entering a folder the first entry under `..` is highlighted; after going up, the folder
  you came from.
- After playing a file the focus stays on the list; the file gets the `>` marker.
- When the file changes by itself (web panel, console) the list scrolls to it only when the list
  is not focused. Changes of the folder or files from the web panel are shown at once.
- Edit mode ends after 8 s without input. After `dimSec` the backlight dims; the first input
  only wakes the screen.

## Settings (NVS)

`lib/PlayerLcd/LcdSettings.def`, NVS namespace `c64lcd`: `brightness` (100 %), `dimLevel`
(15 %), `dimSec` (120 s, 0 = never), `offSec` (0 = never), `theme` (0 = Modern, 1 = Commodore 64).
There is no menu for them yet; change the defaults in the `.def` file or from code
(`lcd.settings()`, `lcd.applySettings()`, `lcd.settings().save()`).

## Software

| File | Contents |
| --- | --- |
| `lib/PlayerLcd/PlayerLcd.*` | `PlayerLcd(Player&)`: `begin()`, `handle()` in `loop()`; focus, edit, list, drawing |
| `lib/PlayerLcd/LcdInput.*` | encoder (hardware PCNT counter) and buttons → events |
| `lib/PlayerLcd/LcdTheme.h` | colours: Modern and Commodore 64 |
| `lib/PlayerLcd/LcdSettings.*` | NVS settings |
| `lib/Player/Player.*` | new `listGeneration()` – lets the LCD notice listing changes made by the web panel |

`handle()` runs in `loop()`, in the same task as the web panel and the console (the Player file
methods must be called from one task). The audio task has a higher priority on the same core, so
drawing never interrupts the sound. Only changed parts are redrawn, through a 320×60 sprite.

Ideas for later: larger list when it is focused, settings menu, jump by first letter,
SID voice meters, fonts with Polish letters.

---

# LCD ILI9341 + enkoder + przyciski (PL)

Opcjonalny panel sterowania dla ESP32-S3 N16R8: wyświetlacz ILI9341 2,8" 320×240 SPI
(biblioteka TFT_eSPI), enkoder z przyciskiem i dwa przyciski (**1 = Wstecz**, **2 = Wybierz**).
Działa razem z panelem WWW i konsolą i pokazuje ten sam stan odtwarzacza (jeden bieżący folder,
jedna głośność).

## Kompilacja

Środowisko `esp32s3-n16r8-lcd` w `platformio.ini` (rozszerza `esp32s3-n16r8`):

```
pio run -e esp32s3-n16r8-lcd -t upload
```

| Flaga | Znaczenie |
| --- | --- |
| `-DUSE_LCD=1` | kompiluje `lib/PlayerLcd` i uruchamia go w `main.cpp` |
| `-DLCD_BUTTONS=1` | enkoder + przyciski Wstecz i Wybierz (domyślnie) |
| `-DLCD_BUTTONS=0` | tylko enkoder: krótko = Wybierz, długo = Wstecz, bardzo długo = Teraz gra |
| `-DTFT_*`, `-DILI9341_DRIVER` ... | konfiguracja TFT_eSPI (bez edycji biblioteki) |

Bez `USE_LCD` nic z LCD się nie kompiluje, a TFT_eSPI nie jest pobierana. Klasyczny ESP32 nie
jest obsługiwany (`config.h` zatrzyma kompilację przez `#error`).

## Podłączenie (ESP32-S3)

Wyświetlacz ma własną magistralę SPI (HSPI), osobną od karty SD (GPIO 10–13).

| Sygnał | GPIO | Gdzie ustawić |
| --- | --- | --- |
| TFT SCK | 14 | `platformio.ini` `-DTFT_SCLK` |
| TFT MOSI (SDI) | 21 | `-DTFT_MOSI` |
| TFT MISO (SDO) | – | niepodłączony |
| TFT CS | 9 | `-DTFT_CS` |
| TFT DC (RS) | 47 | `-DTFT_DC` |
| TFT RESET | 3V3 albo EN | `-DTFT_RST=-1` (reset programowy) |
| TFT LED (podświetlenie) | 2 | `config.h` `LCD_PIN_BL`, PWM, kanał LEDC 0 |
| Enkoder A (CLK) | 40 | `LCD_PIN_ENC_A` |
| Enkoder B (DT) | 41 | `LCD_PIN_ENC_B` |
| Przycisk enkodera | 42 | `LCD_PIN_PUSH` |
| Przycisk 1 – Wstecz | 1 | `LCD_PIN_BACK` |
| Przycisk 2 – Wybierz | 39 | `LCD_PIN_SELECT` |

- Wyświetlacz: VCC 3V3, GND. Podświetlenie pobiera 50–80 mA: LED przez tranzystor
  (np. BC547 z 1 kΩ do bazy albo AO3400). `LCD_PIN_BL -1` = LED na stałe do 3V3, bez przyciemniania.
- Goły enkoder (bez modułu): wspólny pin C i jeden pin przycisku do GND, A/B/przycisk do GPIO.
  Używane są wewnętrzne pull-upy, a drgania styków usuwa sprzętowy filtr PCNT. Gdy gubi kroki,
  dołóż 10 kΩ do 3V3 i 10 nF do GND na A i B.
- Przyciski: między GPIO a GND (wewnętrzny pull-up), opcjonalnie 100 nF.
- Gdy jedno kliknięcie przesuwa o dwa, ustaw `LCD_ENC_COUNTS 2`; zły kierunek: `LCD_ENC_REVERSE 1`.
  Zamienione czerwony i niebieski: odkomentuj `-DTFT_RGB_ORDER=TFT_BGR`.
- Wolne zostają piny 38 i 48 (dioda RGB na DevKitC-1).

## Ekran

Od góry: Teraz gra (tytuł, autor, rok, podutwór, model SID, PAL/NTSC, stan, czas), Głośność,
Źródło, pasek ścieżki z pozycją n/m, lista 6 wierszy (`..`, foldery w nawiasach, pliki z
rozmiarem, `>` = gra teraz). Zaznaczone pole ma ramkę, edytowane pole ciemniejsze tło, wiersz
pod kursorem jest wypełniony. Błędy i „Wczytywanie...” pojawiają się na kilka sekund w pasku
ścieżki. Długie nazwy są skracane „...”, a nazwa pod kursorem po 1 s przewija się w poziomie.
Wbudowane czcionki mają tylko ASCII, więc polskie litery i Latin-1 są bez ogonków (ą → a, ö → o).

## Sterowanie

| Gdzie | Enkoder | Wybierz (2) | Wstecz (1) |
| --- | --- | --- | --- |
| Przechodzenie między polami | następne / poprzednie pole | edycja pola | – |
| Teraz gra (edycja) | poprzedni / następny podutwór | koniec edycji | koniec edycji |
| Głośność (edycja) | głośniej / ciszej, od razu słychać; szybki obrót = 5 | koniec edycji | koniec edycji |
| Źródło (edycja) | wybór źródła (tylko podgląd) | zatwierdź, wczytaj listę, przejdź na listę | anuluj |
| Lista | poprzedni / następny wiersz; szybki obrót = 5 wierszy; w górę na pierwszym wierszu = pole Źródło | plik: graj; folder: wejdź; `..`: folder wyżej | folder wyżej; w katalogu głównym: wyjście z listy |

| Dodatkowo | Enkoder + 2 przyciski | Tylko enkoder (`LCD_BUTTONS=0`) |
| --- | --- | --- |
| Wybierz | przycisk 2 | krótkie wciśnięcie enkodera |
| Wstecz | przycisk 1 | długie wciśnięcie (≥ 0,6 s), po puszczeniu |
| Powrót na Teraz gra | długie wciśnięcie 1 | bardzo długie wciśnięcie (≥ 2 s) |
| Pauza / wznów | krótkie wciśnięcie enkodera | Wybierz na polu Teraz gra |
| Stop | długie wciśnięcie enkodera | panel WWW / konsola |
| Zmiana podutworu | edycja pola Teraz gra | panel WWW / konsola |

- Po wejściu do folderu podświetlony jest pierwszy wpis pod `..`; po wyjściu wyżej – folder,
  z którego się wyszło.
- Po zagraniu pliku fokus zostaje na liście, a plik dostaje znacznik `>`.
- Gdy plik zmieni się sam (WWW, konsola), lista przewija się do niego tylko wtedy, gdy nie jest
  zaznaczona. Zmiany folderu i plików z panelu WWW są widoczne od razu.
- Edycja kończy się po 8 s bez ruchu. Po `dimSec` podświetlenie przygasa; pierwszy ruch tylko
  budzi ekran.

## Ustawienia (NVS)

`lib/PlayerLcd/LcdSettings.def`, przestrzeń NVS `c64lcd`: `brightness` (100 %), `dimLevel`
(15 %), `dimSec` (120 s, 0 = nigdy), `offSec` (0 = nigdy), `theme` (0 = Nowoczesny, 1 = Commodore 64).
Menu ustawień jeszcze nie ma; wartości domyślne zmienia się w pliku `.def` albo w kodzie
(`lcd.settings()`, `lcd.applySettings()`, `lcd.settings().save()`).

## Oprogramowanie

| Plik | Zawartość |
| --- | --- |
| `lib/PlayerLcd/PlayerLcd.*` | `PlayerLcd(Player&)`: `begin()`, `handle()` w `loop()`; fokus, edycja, lista, rysowanie |
| `lib/PlayerLcd/LcdInput.*` | enkoder (sprzętowy licznik PCNT) i przyciski → zdarzenia |
| `lib/PlayerLcd/LcdTheme.h` | kolory: Nowoczesny i Commodore 64 |
| `lib/PlayerLcd/LcdSettings.*` | ustawienia w NVS |
| `lib/Player/Player.*` | nowe `listGeneration()` – LCD zauważa zmiany listy zrobione z panelu WWW |

`handle()` działa w `loop()`, w tym samym zadaniu co panel WWW i konsola (metody plików Playera
muszą być wołane z jednego zadania). Zadanie audio ma wyższy priorytet na tym samym rdzeniu, więc
rysowanie nie przerywa dźwięku. Przerysowywane są tylko zmienione części, przez sprite 320×60.

Pomysły na później: większa lista, gdy jest zaznaczona, menu ustawień, skok do pierwszej litery,
wskaźniki głosów SID, czcionki z polskimi literami.
