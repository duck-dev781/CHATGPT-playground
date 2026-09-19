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

  Libraries:
    - ESP32-A2DP by Phil Schatzmann
    - Arduino Audio Tools by Phil Schatzmann

  This is Bluetooth Classic A2DP, NOT BLE.
*/

#include <Arduino.h>
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
  Serial.println("Bluetooth Classic A2DP + AudioTools");
  Serial.println("ESP32 Arduino Core 3.x / 3.3.12");
  Serial.println("========================================");

  // Configure AudioTools I2S output for the Freenove PCM5102A.
  auto cfg = i2s.defaultConfig();
  cfg.pin_bck = I2S_BCK;
  cfg.pin_ws = I2S_LRCK;
  cfg.pin_data = I2S_DOUT;

  // A2DP music is normally 44.1 kHz stereo 16-bit.
  cfg.sample_rate = 44100;
  cfg.bits_per_sample = 16;
  cfg.channels = 2;

  i2s.begin(cfg);

  Serial.println("PCM5102A I2S output initialized.");
  Serial.println("Starting Bluetooth Classic A2DP...");

  // No BLE is required. This creates an A2DP music receiver.
  a2dp_sink.start("LOud! Com.");

  Serial.println("Bluetooth name: LOud! Com.");
  Serial.println("Use your phone/Chromebook Bluetooth settings to connect.");
  Serial.println("Then YouTube / YouTube Music audio will stream to the PCM5102A.");
}

void loop() {
  delay(100);
}
