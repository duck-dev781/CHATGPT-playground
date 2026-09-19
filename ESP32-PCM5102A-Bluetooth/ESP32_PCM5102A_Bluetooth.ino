/*
  ESP32 Bluetooth Speaker - Freenove PCM5102A
  Arduino ESP32 Core 3.x / 3.3.12

  Audio path:
    Phone / Chromebook / YouTube
      -> Bluetooth Classic A2DP
      -> ESP32
      -> AudioTools I2SStream
      -> PCM5102A
      -> amplifier / speaker

  PCM5102A wiring:
    BCK  -> GPIO 26
    LCK/LRCK/WS -> GPIO 25
    DIN  -> GPIO 22
    SCK  -> NOT CONNECTED
    VCC  -> 5V
    GND  -> GND

  REQUIRED LIBRARIES:
    1. ESP32-A2DP by Phil Schatzmann
    2. AudioTools by Phil Schatzmann

  IMPORTANT:
    This is Bluetooth Classic A2DP, NOT BLE.
*/

#include <Arduino.h>

// AudioTools MUST be included before BluetoothA2DPSink.
// ESP32-A2DP detects AudioTools at compile time and uses its
// modern I2S output path on ESP32 Arduino Core 3.x.
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

// Freenove PCM5102A pins
static const int I2S_BCK  = 26;
static const int I2S_LRCK = 25;
static const int I2S_DOUT = 22;

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("LOud! Com. - ESP32 Bluetooth Speaker");
  Serial.println("Bluetooth Classic A2DP + AudioTools I2S");
  Serial.println("ESP32 Arduino Core 3.x / 3.3.12");
  Serial.println("========================================");

  // Configure the AudioTools I2S output.
  auto cfg = i2s.defaultConfig();

  cfg.pin_bck  = I2S_BCK;
  cfg.pin_ws   = I2S_LRCK;
  cfg.pin_data = I2S_DOUT;

  cfg.sample_rate = 44100;
  cfg.bits_per_sample = 16;
  cfg.channels = 2;

  // Larger buffering helps prevent unstable I2S output/ticking.
  cfg.buffer_size = 64;
  cfg.buffer_count = 8;

  if (!i2s.begin(cfg)) {
    Serial.println("ERROR: AudioTools I2S initialization failed!");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("AudioTools I2S initialized.");
  Serial.println("BCK=26  WS=25  DIN=22");
  Serial.println("Starting Bluetooth Classic A2DP...");

  // Explicitly force Classic Bluetooth to remain connectable
  // and generally discoverable.
  a2dp_sink.set_discoverability(ESP_BT_GENERAL_DISCOVERABLE);
  a2dp_sink.set_connectable(true);

  // This is a Bluetooth MUSIC RECEIVER, so phone/Chromebook
  // audio such as YouTube can stream to it.
  a2dp_sink.start("LOud! Com.");

  Serial.println();
  Serial.println("========================================");
  Serial.println("Bluetooth name: LOud! Com.");
  Serial.println("Bluetooth mode: Classic A2DP");
  Serial.println("Discoverable: YES");
  Serial.println("Connect from your phone/Chromebook.");
  Serial.println("YouTube audio -> ESP32 -> PCM5102A");
  Serial.println("========================================");
}

void loop() {
  // Keep the Bluetooth/A2DP stack running.
  delay(100);
}
