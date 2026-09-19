/*
  ESP32 Bluetooth Speaker - Freenove PCM5102A
  Arduino ESP32 Core 3.x / 3.3.12

  Freenove PCM5102A wiring:
    PCM5102A BCK  -> ESP32 GPIO 26
    PCM5102A LCK  -> ESP32 GPIO 25
    PCM5102A DIN  -> ESP32 GPIO 22
    PCM5102A SCK  -> NOT CONNECTED
    PCM5102A VCC  -> 5V
    PCM5102A GND  -> GND

  Install the "ESP32-A2DP" library by Phil Schatzmann
  before compiling this sketch.
*/

#include <Arduino.h>
#include "BluetoothA2DPSink.h"
#include "ESP_I2S.h"

// Freenove PCM5102A pins
static const uint8_t I2S_BCK  = 26;
static const uint8_t I2S_LRCK = 25;
static const uint8_t I2S_DOUT = 22;

I2SClass i2s;
BluetoothA2DPSink a2dp_sink(i2s);

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("====================================");
  Serial.println("Freenove ESP32 PCM5102A Bluetooth");
  Serial.println("ESP32 Arduino Core 3.x");
  Serial.println("====================================");

  // Configure the Arduino-ESP32 3.x I2S peripheral.
  i2s.setPins(I2S_BCK, I2S_LRCK, I2S_DOUT);

  if (!i2s.begin(
        I2S_MODE_STD,
        44100,
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_STEREO,
        I2S_STD_SLOT_BOTH)) {
    Serial.println("ERROR: I2S initialization failed!");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("I2S initialized.");
  Serial.println("Starting Bluetooth A2DP...");

  // Bluetooth device name shown on your phone/Chromebook.
  a2dp_sink.start("LOud! Com.", true);

  Serial.println("Bluetooth device: LOud! Com.");
  Serial.println("Connect to it and play music.");
}

void loop() {
  delay(1000);
}
