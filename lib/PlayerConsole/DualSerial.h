/**
 * @file DualSerial.h
 * @brief DualSerial - console on two ports at once.
 *
 * On the ESP32-S3 with USB CDC (-DARDUINO_USB_CDC_ON_BOOT=1) "Serial" is the native USB socket, and "Serial0"
 * is the UART ("COM" socket); commands work regardless of where the cable is plugged in.
 * Without USB CDC, DualSerial simply forwards everything to Serial.
 */
#pragma once
#include <Arduino.h>

/**
 * @brief Stream that reads from and writes to both Serial (USB CDC) and Serial0 (UART).
 */
class DualSerial : public Stream {
public:
    /**
     * @brief Starts the port(s).
     * @param baud Baud rate.
     */
    void begin(unsigned long baud) {
        Serial.begin(baud);
#if ARDUINO_USB_CDC_ON_BOOT
        Serial.setTxTimeoutMs(0);    // nie blokuj, gdy nikt nie slucha na USB
        Serial0.begin(baud);
#endif
    }
#if ARDUINO_USB_CDC_ON_BOOT
    /** @brief Bytes waiting on both ports. @return Total count. */
    int available() override { return Serial.available() + Serial0.available(); }
    /** @brief Reads a byte, USB first. @return Byte, or -1 when none. */
    int read() override { return Serial.available() ? Serial.read() : Serial0.read(); }
    /** @brief Peeks at the next byte, USB first. @return Byte, or -1 when none. */
    int peek() override { return Serial.available() ? Serial.peek() : Serial0.peek(); }
    /** @brief Waits until the UART output is sent. */
    void flush() override { Serial0.flush(); }
    /**
     * @brief Writes a byte to both ports.
     * @param c Byte.
     * @return 1.
     */
    size_t write(uint8_t c) override { Serial0.write(c); Serial.write(c); return 1; }
    /**
     * @brief Writes a block to both ports.
     * @param buf Data.
     * @param n   Length.
     * @return n.
     */
    size_t write(const uint8_t* buf, size_t n) override { Serial0.write(buf, n); Serial.write(buf, n); return n; }
#else
    /** @brief Bytes waiting on Serial. @return Count. */
    int available() override { return Serial.available(); }
    /** @brief Reads a byte. @return Byte, or -1 when none. */
    int read() override { return Serial.read(); }
    /** @brief Peeks at the next byte. @return Byte, or -1 when none. */
    int peek() override { return Serial.peek(); }
    /** @brief Waits until the output is sent. */
    void flush() override { Serial.flush(); }
    /**
     * @brief Writes a byte.
     * @param c Byte.
     * @return Bytes written.
     */
    size_t write(uint8_t c) override { return Serial.write(c); }
    /**
     * @brief Writes a block.
     * @param buf Data.
     * @param n   Length.
     * @return Bytes written.
     */
    size_t write(const uint8_t* buf, size_t n) override { return Serial.write(buf, n); }
#endif
};
