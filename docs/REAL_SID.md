# Real SID chip (MOS 6581 / 8580) connected to ESP32

*[Wersja polska poniżej](#prawdziwy-układ-sid-mos-6581--8580-podłączony-do-esp32)*

Instead of emulation, the music is played by the original C64 sound chip. The ESP32 still emulates
the 6510 CPU and runs the music code from the `.sid` file, but register writes to `$D400–$D418`
are sent to the real SID. Sound is taken from the chip's AUDIO OUT pin, not from the I2S DAC.

Block diagram:

```
            3.3 V logic                     5 V logic                     SID 6581/8580
ESP32 ──DATA──► 74HCT595 #1 (D0–D7) ──Q0..Q7──────────────────────────► D0–D7  (15–22)
      ──SHIFT─► SHCP #1 and #2    │Q7'
      ──LATCH─► STCP #1 and #2    └──► 74HCT595 #2 (A0–A4) ──Q0..Q4──► A0–A4  (9–13)
      ──CS────► 74HCT125 ───────────────────────────────────────────► /CS    (8)
      ──PHI2──► 74HCT125 ───────────────────────────────────────────► Φ2     (6)
      ──RES───► 74HCT125 ───────────────────────────────────────────► /RES   (5)
                                                          GND ────────► R/W   (7)
                                                                        AUDIO OUT (27) ──► filter / amplifier
```

## 1. Parts list

| Part | Qty | Notes |
|---|---|---|
| SID 6581 or 8580 | 1 | in a DIP-28 socket (do not solder the chip in) |
| 74**HCT**595 | 2 | shift registers; must be the **HCT** series (see below) |
| 74**HCT**125 | 1 | buffer for /CS, Φ2, /RES (74HCT244 / 74HCT541 also work) |
| SID filter capacitors | 2 | **6581: 470 pF**, **8580: 22 nF** (ceramic / film) |
| 3PDT switch (or 3 jumpers) | 1 | 6581 / 8580 selection: VDD and both capacitors at once |
| 7809 regulator | 1 | VDD for 8580 |
| 12 V → 5 V step-down converter (MP1584, LM2596) or 7805 with heatsink | 1 | 5 V for SID, 74HCT and ESP32 |
| Regulated 12 V DC power supply, min. 1 A | 1 | VDD for 6581 (better: 15 V + 7812, see power supply) |
| 100 nF capacitors | 6 | decoupling at every chip |
| 47–100 µF / 25 V electrolytic capacitors | 3 | on 12 V, 9 V, 5 V |
| 1 kΩ resistor | 1 | AUDIO OUT load |
| 10 µF / 25 V capacitor | 1 | audio output coupling |
| 1 µF capacitor | 1 | EXT IN to ground |
| 10 kΩ resistors | 2 | pull /RES to ground and /CS to 3.3 V |
| Amplifier (PAM8403, LM386) or line input | 1 | |

## 2. SID pinout (DIP-28, top view, notch up)

| Pin | Signal | Connection | | Pin | Signal | Connection |
|---|---|---|---|---|---|---|
| 1 | CAP1A | filter capacitor 1 | | 28 | VDD | **12 V (6581) / 9 V (8580)** |
| 2 | CAP1B | filter capacitor 1 | | 27 | AUDIO OUT | 1 kΩ to GND, audio output |
| 3 | CAP2A | filter capacitor 2 | | 26 | EXT IN | 1 µF to GND |
| 4 | CAP2B | filter capacitor 2 | | 25 | VCC | **5 V** |
| 5 | /RES | 74HCT125 3Y | | 24 | POT X | not connected |
| 6 | Φ2 | 74HCT125 2Y | | 23 | POT Y | not connected |
| 7 | R/W | **GND** (write only) | | 22 | D7 | 595 #1 Q7 |
| 8 | /CS | 74HCT125 1Y | | 21 | D6 | 595 #1 Q6 |
| 9 | A0 | 595 #2 Q0 | | 20 | D5 | 595 #1 Q5 |
| 10 | A1 | 595 #2 Q1 | | 19 | D4 | 595 #1 Q4 |
| 11 | A2 | 595 #2 Q2 | | 18 | D3 | 595 #1 Q3 |
| 12 | A3 | 595 #2 Q3 | | 17 | D2 | 595 #1 Q2 |
| 13 | A4 | 595 #2 Q4 | | 16 | D1 | 595 #1 Q1 |
| 14 | GND | ground | | 15 | D0 | 595 #1 Q0 |

## 3. ESP32 connections

Default pins (can be changed in `include/config.h`, `REAL_SID_PIN_*`). They do not collide with
the I2S DAC (4/5/6), the SD card (10–13), flash/PSRAM (26–37 on N16R8), USB (19/20) or the
strapping pins (0, 3, 45, 46).

| Signal | ESP32-S3 | ESP32 | Goes to |
|---|---|---|---|
| DATA | GPIO15 | GPIO13 | 74HCT595 #1, pin 14 (DS) |
| SHIFT | GPIO16 | GPIO14 | 74HCT595 #1 and #2, pin 11 (SHCP) |
| LATCH | GPIO17 | GPIO27 | 74HCT595 #1 and #2, pin 12 (STCP) |
| CS | GPIO18 | GPIO21 | 74HCT125 pin 2 (1A) → pin 3 (1Y) → SID pin 8 |
| PHI2 | GPIO7 | GPIO32 | 74HCT125 pin 5 (2A) → pin 6 (2Y) → SID pin 6 |
| RES | GPIO8 | GPIO33 | 74HCT125 pin 9 (3A) → pin 8 (3Y) → SID pin 5 |
| GND | GND | GND | common ground with the whole board |

### 74HCT595 – both chips

| 595 pin | #1 (data) | #2 (address) |
|---|---|---|
| 16 VCC | 5 V | 5 V |
| 8 GND | GND | GND |
| 10 /MR | 5 V | 5 V |
| 13 /OE | GND | GND |
| 14 DS | ESP32 DATA | **595 #1 pin 9 (Q7')** |
| 11 SHCP | ESP32 SHIFT | ESP32 SHIFT |
| 12 STCP | ESP32 LATCH | ESP32 LATCH |
| 15 Q0 | SID D0 (15) | SID A0 (9) |
| 1 Q1 | SID D1 (16) | SID A1 (10) |
| 2 Q2 | SID D2 (17) | SID A2 (11) |
| 3 Q3 | SID D3 (18) | SID A3 (12) |
| 4 Q4 | SID D4 (19) | SID A4 (13) |
| 5 Q5 | SID D5 (20) | – |
| 6 Q6 | SID D6 (21) | – |
| 7 Q7 | SID D7 (22) | – |
| 9 Q7' | to DS of chip #2 | – |

The firmware shifts 16 bits MSB first: the address first (it moves on to #2), then the data (stays in #1).

### 74HCT125

| Pin | Connection |
|---|---|
| 14 VCC | 5 V |
| 7 GND | GND |
| 1, 4, 10 (/OE 1–3) | GND (buffers always enabled) |
| 2 (1A) / 3 (1Y) | ESP32 CS → SID /CS (8); **10 kΩ from 1A to 3.3 V** |
| 5 (2A) / 6 (2Y) | ESP32 PHI2 → SID Φ2 (6) |
| 9 (3A) / 8 (3Y) | ESP32 RES → SID /RES (5); **10 kΩ from 3A to GND** |
| 13 (/OE4) | 5 V (unused), 12 (4A) to GND |

The resistors keep the SID in reset and deselected until the ESP32 sets up its pins, so the chip
stays silent at power-on.

### Why HCT and why there is no level shifter

- A 74HC powered from 5 V needs about 3.5 V for a high level – 3.3 V from the ESP32 is not enough.
  74**HCT** has a TTL threshold (2.0 V), so it accepts 3.3 V and drives full 5 V to the SID.
- R/W is tied to ground, so the SID never drives the data bus and no 5 V ever reaches the ESP32.
  The ESP32 is connected only to HCT inputs – nothing can damage it.
- Consequence: SID registers cannot be read. Reads of `$D419–$D41C` (OSC3, ENV3, paddles)
  are answered by reSID running in the background (good enough e.g. for random generators in music).

## 4. Power supply

| Voltage | 6581 | 8580 | From |
|---|---|---|---|
| VCC (pin 25) | 5 V | 5 V | 12 V → 5 V converter |
| VDD (pin 28) | **12 V** | **9 V** | 12 V supply / 7809 |

- **Warning: an 8580 can be damaged at 12 V.** A 6581 at 9 V only plays quieter and with noise.
  Set the switch to 8580 (9 V) by default.
- Better (quieter) option: a 15 V supply and two linear regulators 7812 / 7809. A 12 V switching
  supply connected straight to VDD can be heard as noise – then add a 100 µH choke and 100 µF
  before the VDD pin.
- Feed 5 V to the 5V pin of the ESP32 board. **Do not connect USB and the external 5 V at the
  same time**, unless through a Schottky diode (e.g. 1N5819) on the converter side.
- 100 nF between VCC and GND at every chip, as close to the pins as possible. At the SID also
  100 nF + 47 µF on VDD.

### Model switch (one board for 6581 and 8580)

A 3PDT switch (or three jumpers) changes at once:

| Section | 6581 position | 8580 position |
|---|---|---|
| A: VDD (pin 28) | 12 V | 7809 output (9 V) |
| B: CAP1A–CAP1B (1–2) | 470 pF | 22 nF |
| C: CAP2A–CAP2B (3–4) | 470 pF | 22 nF |

After changing the chip also set `REAL_SID_MODEL` in `include/config.h` (0 = 6581, 1 = 8580)
and upload the firmware again. The background emulation needs the model (OSC3/ENV3 reads), and
it is shown as the engine name (`prawdziwy SID 8580`).

## 5. Audio output

```
                   10 µF (+ to SID)     1 kΩ (optional)
SID pin 27 ──┬────────||──────────────/\/\/──┬──────► 10 kΩ potentiometer (volume) ──► amplifier
             │                               │
           1 kΩ                            10 nF (optional, ~16 kHz filter)
             │                               │
            GND                             GND
```

- The 1 kΩ to ground is required – the SID output is an open-source transistor (same as in the C64).
- The output carries a few volts of DC (6581 more than 8580) – the 10 µF capacitor blocks it,
  positive side towards the SID.
- Then a line input (amplifier, computer) or a small PAM8403 / LM386 amplifier with a speaker.
  With a long cable add a buffer (a BC547 transistor as in the C64, or a rail-to-rail op-amp).
- Route the audio ground separately and join it with the digital ground at one point (at the supply).
- EXT IN (pin 26): 1 µF to ground, do not leave it floating.

## 6. Firmware

Enable it in `platformio.ini`, in `build_flags` of your environment:

```ini
    -DSID_ENGINE=2
```

Settings in `include/config.h`:

```c
#define REAL_SID_MODEL      1    // 0 = 6581, 1 = 8580 (chip in the socket)
#define REAL_SID_EMU_AUDIO  0    // 1 = reSID also plays through the I2S DAC (comparison)
// #define REAL_SID_PIN_CS  18   // other pins if the defaults are taken
```

Rarely needed (with `-D` in `platformio.ini` or in `config.h`): `REAL_SID_LATENCY_MS` (30),
`REAL_SID_TASK_CORE` (0), `REAL_SID_TASK_PRIORITY` (3), `REAL_SID_QUEUE_LEN` (1024).

How it works (`lib/SidEngine/real_sid_chip.*`):

- The emulated 6510 computes a whole frame at once, so a SID write first goes into a queue with
  its time in C64 cycles. A separate task (core 0) sends it to the chip with the same spacing as
  on a C64, with a delay of about 30 ms (accurate to a few µs, which matters for digi tunes).
- A write: 16 bits into the 595 registers, a LATCH pulse, then /CS low for 2 µs – the SID latches
  the register on the falling edge of Φ2. One write takes about 3 µs.
- Φ2 comes from the LEDC hardware generator: 985 248 Hz for PAL files, 1 022 727 Hz for NTSC
  (changed when a tune starts).
- At the start of every tune: /RES low for 20 µs and all registers cleared.
- Pause, stop and mute: the firmware sets the volume in register `$D418` to 0, so the chip does
  not keep playing the last note. It is restored on resume.
- The volume slider in the web panel only affects I2S sound. Set the real SID's volume with
  a potentiometer at the output or on the amplifier.
- The model override (`model 6581/8580` in the console and web panel) has no effect – the chip
  in the socket plays. Tunes written for the other model will play, but the filter and some
  waveforms will sound different.

## 7. First power-on

1. Without the SID in the socket: power on and measure 5 V on pin 25 and 9 V / 12 V on pin 28
   of the socket (according to the switch).
2. Upload the firmware with `-DSID_ENGINE=2` and play any file. A multimeter on socket pin 6 shows
   about 2.5 V (1 MHz square wave, 5 V, 50 % duty), pin 5 shows 5 V (reset released).
3. Power off, insert the SID (notch matching the socket), power on and play `demo_gama.sid`.
4. No sound: check the 1 kΩ on pin 27, R/W to ground, the Q7' → DS #2 chain, that the chips are
   HCT, and that the serial monitor shows the engine as `prawdziwy SID ...`.
5. Hiss / hum: VDD from a switching supply – add an LC filter or a linear regulator.

Limitations: one SID at `$D400` (no 2SID/3SID), register writes only.

---

# Prawdziwy układ SID (MOS 6581 / 8580) podłączony do ESP32

Zamiast emulacji muzykę gra oryginalny układ dźwiękowy z C64. ESP32 dalej emuluje procesor 6510
i wykonuje kod muzyki z pliku `.sid`, ale zapisy do rejestrów `$D400–$D418` wysyła do prawdziwego
SID-a. Dźwięk bierzesz z nóżki AUDIO OUT układu, a nie z DAC I2S.

Rysunek ideowy:

```
            3,3 V logika                    5 V logika                    SID 6581/8580
ESP32 ──DATA──► 74HCT595 #1 (D0–D7) ──Q0..Q7──────────────────────────► D0–D7  (15–22)
      ──SHIFT─► SHCP #1 i #2      │Q7'
      ──LATCH─► STCP #1 i #2      └──► 74HCT595 #2 (A0–A4) ──Q0..Q4──► A0–A4  (9–13)
      ──CS────► 74HCT125 ───────────────────────────────────────────► /CS    (8)
      ──PHI2──► 74HCT125 ───────────────────────────────────────────► Φ2     (6)
      ──RES───► 74HCT125 ───────────────────────────────────────────► /RES   (5)
                                                          GND ────────► R/W   (7)
                                                                        AUDIO OUT (27) ──► filtr / wzmacniacz
```

## 1. Lista części

| Część | Ilość | Uwagi |
|---|---|---|
| SID 6581 albo 8580 | 1 | w podstawce DIP-28 (nie lutuj układu na stałe) |
| 74**HCT**595 | 2 | rejestry przesuwne; koniecznie seria **HCT** (patrz niżej) |
| 74**HCT**125 | 1 | bufor dla /CS, Φ2, /RES (może być też 74HCT244 / 74HCT541) |
| Kondensatory filtra SID | 2 | **6581: 470 pF**, **8580: 22 nF** (ceramiczne / foliowe) |
| Przełącznik 3PDT (albo 3 zworki) | 1 | wybór 6581 / 8580: VDD i oba kondensatory naraz |
| Stabilizator 7809 | 1 | VDD dla 8580 |
| Przetwornica step-down 12 V → 5 V (MP1584, LM2596) albo 7805 z radiatorem | 1 | 5 V dla SID, 74HCT i ESP32 |
| Zasilacz 12 V DC, min. 1 A, stabilizowany | 1 | VDD dla 6581 (lepiej: 15 V + 7812, patrz zasilanie) |
| Kondensatory 100 nF | 6 | odsprzęganie przy każdym układzie |
| Kondensatory elektrolityczne 47–100 µF / 25 V | 3 | na 12 V, 9 V, 5 V |
| Opornik 1 kΩ | 1 | obciążenie wyjścia AUDIO OUT |
| Kondensator 10 µF / 25 V | 1 | sprzęgający na wyjściu audio |
| Kondensator 1 µF | 1 | EXT IN do masy |
| Oporniki 10 kΩ | 2 | podciąganie /RES do masy i /CS do 3,3 V |
| Wzmacniacz (PAM8403, LM386) albo wejście liniowe | 1 | |

## 2. Wyprowadzenia SID (DIP-28, widok z góry, wycięcie u góry)

| Pin | Sygnał | Podłączenie | | Pin | Sygnał | Podłączenie |
|---|---|---|---|---|---|---|
| 1 | CAP1A | kondensator filtra 1 | | 28 | VDD | **12 V (6581) / 9 V (8580)** |
| 2 | CAP1B | kondensator filtra 1 | | 27 | AUDIO OUT | 1 kΩ do GND, wyjście audio |
| 3 | CAP2A | kondensator filtra 2 | | 26 | EXT IN | 1 µF do GND |
| 4 | CAP2B | kondensator filtra 2 | | 25 | VCC | **5 V** |
| 5 | /RES | 74HCT125 3Y | | 24 | POT X | niepodłączony |
| 6 | Φ2 | 74HCT125 2Y | | 23 | POT Y | niepodłączony |
| 7 | R/W | **GND** (tylko zapis) | | 22 | D7 | 595 #1 Q7 |
| 8 | /CS | 74HCT125 1Y | | 21 | D6 | 595 #1 Q6 |
| 9 | A0 | 595 #2 Q0 | | 20 | D5 | 595 #1 Q5 |
| 10 | A1 | 595 #2 Q1 | | 19 | D4 | 595 #1 Q4 |
| 11 | A2 | 595 #2 Q2 | | 18 | D3 | 595 #1 Q3 |
| 12 | A3 | 595 #2 Q3 | | 17 | D2 | 595 #1 Q2 |
| 13 | A4 | 595 #2 Q4 | | 16 | D1 | 595 #1 Q1 |
| 14 | GND | masa | | 15 | D0 | 595 #1 Q0 |

## 3. Połączenia z ESP32

Domyślne piny (można zmienić w `include/config.h`, `REAL_SID_PIN_*`). Nie kolidują z DAC I2S
(4/5/6), kartą SD (10–13), pamięcią flash/PSRAM (26–37 w N16R8), USB (19/20) ani pinami
startowymi (0, 3, 45, 46).

| Sygnał | ESP32-S3 | ESP32 | Dokąd |
|---|---|---|---|
| DATA | GPIO15 | GPIO13 | 74HCT595 #1, pin 14 (DS) |
| SHIFT | GPIO16 | GPIO14 | 74HCT595 #1 i #2, pin 11 (SHCP) |
| LATCH | GPIO17 | GPIO27 | 74HCT595 #1 i #2, pin 12 (STCP) |
| CS | GPIO18 | GPIO21 | 74HCT125 pin 2 (1A) → pin 3 (1Y) → SID pin 8 |
| PHI2 | GPIO7 | GPIO32 | 74HCT125 pin 5 (2A) → pin 6 (2Y) → SID pin 6 |
| RES | GPIO8 | GPIO33 | 74HCT125 pin 9 (3A) → pin 8 (3Y) → SID pin 5 |
| GND | GND | GND | wspólna masa z całą płytką |

### 74HCT595 – oba układy

| Pin 595 | #1 (dane) | #2 (adres) |
|---|---|---|
| 16 VCC | 5 V | 5 V |
| 8 GND | GND | GND |
| 10 /MR | 5 V | 5 V |
| 13 /OE | GND | GND |
| 14 DS | ESP32 DATA | **595 #1 pin 9 (Q7')** |
| 11 SHCP | ESP32 SHIFT | ESP32 SHIFT |
| 12 STCP | ESP32 LATCH | ESP32 LATCH |
| 15 Q0 | SID D0 (15) | SID A0 (9) |
| 1 Q1 | SID D1 (16) | SID A1 (10) |
| 2 Q2 | SID D2 (17) | SID A2 (11) |
| 3 Q3 | SID D3 (18) | SID A3 (12) |
| 4 Q4 | SID D4 (19) | SID A4 (13) |
| 5 Q5 | SID D5 (20) | – |
| 6 Q6 | SID D6 (21) | – |
| 7 Q7 | SID D7 (22) | – |
| 9 Q7' | do DS układu #2 | – |

Program wysyła 16 bitów od najstarszego: najpierw adres (przechodzi dalej do #2), potem dane (zostają w #1).

### 74HCT125

| Pin | Podłączenie |
|---|---|
| 14 VCC | 5 V |
| 7 GND | GND |
| 1, 4, 10 (/OE 1–3) | GND (bufory zawsze włączone) |
| 2 (1A) / 3 (1Y) | ESP32 CS → SID /CS (8); **10 kΩ z 1A do 3,3 V** |
| 5 (2A) / 6 (2Y) | ESP32 PHI2 → SID Φ2 (6) |
| 9 (3A) / 8 (3Y) | ESP32 RES → SID /RES (5); **10 kΩ z 3A do GND** |
| 13 (/OE4) | 5 V (nieużywany), 12 (4A) do GND |

Oporniki trzymają SID-a w resecie i bez wyboru, zanim ESP32 ustawi swoje piny, więc po włączeniu
zasilania układ milczy.

### Dlaczego HCT i czemu nie ma konwertera poziomów

- 74HC zasilany z 5 V uznaje stan wysoki dopiero od ok. 3,5 V – 3,3 V z ESP32 to za mało.
  74**HCT** ma próg TTL (2,0 V), więc przyjmuje 3,3 V i wystawia pełne 5 V do SID-a.
- R/W jest na stałe na masie, więc SID nigdy nie wystawia danych na szynę i żadne 5 V nie wraca do ESP32.
  Do ESP32 dochodzą tylko wejścia układów HCT – nic go nie uszkodzi.
- Konsekwencja: rejestrów SID-a nie da się czytać. Odczyty `$D419–$D41C` (OSC3, ENV3, potencjometry)
  zwraca emulacja reSID działająca w tle (to wystarcza np. do generatorów losowych w muzyce).

## 4. Zasilanie

| Napięcie | 6581 | 8580 | Skąd |
|---|---|---|---|
| VCC (pin 25) | 5 V | 5 V | przetwornica 12 V → 5 V |
| VDD (pin 28) | **12 V** | **9 V** | 12 V z zasilacza / 7809 |

- **Uwaga: 8580 przy 12 V może się uszkodzić.** 6581 przy 9 V tylko gra ciszej i z zakłóceniami.
  Fabrycznie ustaw przełącznik na 8580 (9 V).
- Lepszy (cichszy) wariant: zasilacz 15 V i dwa stabilizatory liniowe 7812 / 7809. Zasilacz
  impulsowy 12 V podłączony wprost do VDD bywa słychać jako szum – daj wtedy dławik 100 µH
  i 100 µF przed nóżką VDD.
- 5 V dla ESP32 podaj na pin 5V płytki. **Nie podłączaj jednocześnie USB i zewnętrznego 5 V**,
  chyba że przez diodę Schottky (np. 1N5819) od strony przetwornicy.
- Przy każdym układzie 100 nF między VCC a GND, najbliżej nóżek. Przy SID-zie dodatkowo
  100 nF + 47 µF na VDD.

### Przełącznik modelu (jedna płytka dla 6581 i 8580)

Przełącznik 3PDT (albo trzy zworki) zmienia naraz:

| Sekcja | Pozycja 6581 | Pozycja 8580 |
|---|---|---|
| A: VDD (pin 28) | 12 V | wyjście 7809 (9 V) |
| B: CAP1A–CAP1B (1–2) | 470 pF | 22 nF |
| C: CAP2A–CAP2B (3–4) | 470 pF | 22 nF |

Po zmianie układu ustaw też `REAL_SID_MODEL` w `include/config.h` (0 = 6581, 1 = 8580)
i wgraj program ponownie. Model jest potrzebny emulacji w tle (odczyty OSC3/ENV3) i pokazuje
się jako nazwa silnika (`prawdziwy SID 8580`).

## 5. Wyjście audio

```
                   10 µF (+ do SID)     1 kΩ (opcja)
SID pin 27 ──┬────────||──────────────/\/\/──┬──────► potencjometr 10 kΩ (głośność) ──► wzmacniacz
             │                               │
           1 kΩ                            10 nF (opcja, filtr ~16 kHz)
             │                               │
            GND                             GND
```

- 1 kΩ do masy jest obowiązkowy – wyjście SID-a to otwarte źródło tranzystora (tak samo w C64).
- Na wyjściu jest kilka woltów stałego napięcia (6581 więcej niż 8580) – kondensator 10 µF je odcina,
  plusem w stronę SID-a.
- Dalej wejście liniowe (wzmacniacz, komputer) albo mały wzmacniacz PAM8403 / LM386 z głośnikiem.
  Przy długim kablu daj wtórnik (tranzystor BC547 jak w C64 albo wzmacniacz operacyjny rail-to-rail).
- Masę audio prowadź osobną ścieżką i połącz z masą cyfrową w jednym punkcie (przy zasilaczu).
- EXT IN (pin 26): 1 µF do masy, nie zostawiaj wiszącego.

## 6. Program

Włączenie w `platformio.ini`, w `build_flags` swojego środowiska:

```ini
    -DSID_ENGINE=2
```

Ustawienia w `include/config.h`:

```c
#define REAL_SID_MODEL      1    // 0 = 6581, 1 = 8580 (układ w podstawce)
#define REAL_SID_EMU_AUDIO  0    // 1 = reSID gra równolegle przez DAC I2S (porównanie)
// #define REAL_SID_PIN_CS  18   // inne piny, gdy domyślne są zajęte
```

Rzadziej potrzebne (z `-D` w `platformio.ini` albo w `config.h`): `REAL_SID_LATENCY_MS` (30),
`REAL_SID_TASK_CORE` (0), `REAL_SID_TASK_PRIORITY` (3), `REAL_SID_QUEUE_LEN` (1024).

Jak to działa (`lib/SidEngine/real_sid_chip.*`):

- Emulowany 6510 liczy całą ramkę naraz, więc zapis do SID-a trafia najpierw do kolejki z czasem
  w cyklach C64. Osobne zadanie (rdzeń 0) wysyła go do układu w tych samych odstępach co w C64,
  z opóźnieniem ok. 30 ms (dokładność kilku µs, ważne dla utworów z samplami).
- Zapis: 16 bitów do rejestrów 595, impuls LATCH, potem /CS w stanie niskim przez 2 µs – SID
  zatrzaskuje rejestr na opadającym zboczu Φ2. Jeden zapis trwa ok. 3 µs.
- Zegar Φ2 daje sprzętowy generator LEDC: 985 248 Hz dla plików PAL, 1 022 727 Hz dla NTSC
  (zmienia się przy starcie utworu).
- Przy starcie każdego utworu: /RES w stanie niskim 20 µs i zerowanie wszystkich rejestrów.
- Pauza, stop i wyciszenie: program zeruje głośność w rejestrze `$D418`, żeby układ nie grał
  w kółko ostatniej nuty. Po wznowieniu przywraca ją.
- Suwak głośności w panelu WWW działa tylko na dźwięk z I2S. Głośność prawdziwego SID-a ustawiasz
  potencjometrem na wyjściu albo we wzmacniaczu.
- Wymuszenie modelu (`model 6581/8580` w konsoli i WWW) nie ma wpływu – gra ten układ, który
  jest w podstawce. Pliki pisane pod drugi model zagrają, ale filtr i niektóre przebiegi zabrzmią inaczej.

## 7. Pierwsze uruchomienie

1. Bez SID-a w podstawce: włącz zasilanie, sprawdź miernikiem 5 V na pinie 25 i 9 V / 12 V
   na pinie 28 podstawki (zgodnie z przełącznikiem).
2. Wgraj program z `-DSID_ENGINE=2`, uruchom dowolny plik. Na pinie 6 podstawki miernik pokaże
   ok. 2,5 V (prostokąt 1 MHz, 5 V, wypełnienie 50 %), na pinie 5 – 5 V (koniec resetu).
3. Wyłącz zasilanie, włóż SID-a (wycięcie zgodnie z podstawką), włącz i zagraj `demo_gama.sid`.
4. Brak dźwięku: sprawdź 1 kΩ na pinie 27, R/W na masie, kolejność Q7' → DS #2, czy układy
   to HCT, i czy w monitorze szeregowym silnik nazywa się `prawdziwy SID ...`.
5. Szum / buczenie: zasilanie VDD z zasilacza impulsowego – dodaj filtr LC albo stabilizator liniowy.

Ograniczenia: jeden SID na `$D400` (bez 2SID/3SID), tylko zapis do rejestrów.
