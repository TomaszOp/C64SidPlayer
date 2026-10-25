// DualSerial - konsola na dwoch portach naraz. Na ESP32-S3 z USB CDC
// (-DARDUINO_USB_CDC_ON_BOOT=1) "Serial" to natywne gniazdo USB, a "Serial0"
// to UART (gniazdo "COM"); komendy dzialaja niezaleznie od tego, gdzie jest kabel.
// Bez USB CDC DualSerial po prostu przekazuje wszystko do Serial.
#pragma once
#include <Arduino.h>

class DualSerial : public Stream {
public:
    void begin(unsigned long baud) {
        Serial.begin(baud);
#if ARDUINO_USB_CDC_ON_BOOT
        Serial.setTxTimeoutMs(0);    // nie blokuj, gdy nikt nie slucha na USB
        Serial0.begin(baud);
#endif
    }
#if ARDUINO_USB_CDC_ON_BOOT
    int available() override { return Serial.available() + Serial0.available(); }
    int read() override { return Serial.available() ? Serial.read() : Serial0.read(); }
    int peek() override { return Serial.available() ? Serial.peek() : Serial0.peek(); }
    void flush() override { Serial0.flush(); }
    size_t write(uint8_t c) override { Serial0.write(c); Serial.write(c); return 1; }
    size_t write(const uint8_t* buf, size_t n) override { Serial0.write(buf, n); Serial.write(buf, n); return n; }
#else
    int available() override { return Serial.available(); }
    int read() override { return Serial.read(); }
    int peek() override { return Serial.peek(); }
    void flush() override { Serial.flush(); }
    size_t write(uint8_t c) override { return Serial.write(c); }
    size_t write(const uint8_t* buf, size_t n) override { return Serial.write(buf, n); }
#endif
};
