/*
  ESP32 Bluetooth Speaker - Freenove FNK0047 PCM5102A
  Arduino ESP32 Core 3.x / 3.3.12

  Audio path:
    Phone / Chromebook / YouTube
      -> Bluetooth Classic A2DP
      -> ESP32
      -> I2S audio output
      -> FNK0047 Audio Converter & Amplifier (PCM5102A)
      -> speaker

  FNK0047 PCM5102A wiring:
    BCK  -> GPIO 26
    LCK/LRCK/WS -> GPIO 25
    DIN  -> GPIO 22
    SCK  -> NOT CONNECTED
    VCC  -> 5V
    GND  -> GND

  REQUIRED LIBRARIES:
    1. ESP32-A2DP by Phil Schatzmann
    2. AudioTools by Phil Schatzmann
    3. Freenove ESP32-audioI2S (Audio.h)

  IMPORTANT:
    The Freenove ESP32-audioI2S library is the library used by
    the FNK0047 examples for the PCM5102A Audio Converter &
    Amplifier. It is included here so the project has the same
    audio-library dependency as the FNK0047 kit.

    Bluetooth Classic A2DP still supplies the live PCM stream.
    AudioTools provides the Arduino Core 3.x-compatible I2S
    output used by ESP32-A2DP. AudioI2S is not used as an MP3
    decoder here because the source is live Bluetooth audio,
    not an SD-card audio file.

    This is Bluetooth Classic A2DP, NOT BLE.
*/

#include <Arduino.h>

// Freenove FNK0047 audio library.
// This is the ESP32-audioI2S library from the FNK0047 kit.
#include "Audio.h"

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
  Serial.println("LOud! Com. - FNK0047 Bluetooth Speaker");
  Serial.println("PCM5102A + ESP32-audioI2S + AudioTools");
  Serial.println("Bluetooth Classic A2DP");
  Serial.println("ESP32 Arduino Core 3.x / 3.3.12");
  Serial.println("========================================");

  // Configure the AudioTools I2S output for the exact
  // FNK0047 PCM5102A wiring.
  auto cfg = i2s.defaultConfig();

  cfg.pin_bck  = I2S_BCK;
  cfg.pin_ws   = I2S_LRCK;
  cfg.pin_data = I2S_DOUT;

  // 48 kHz is the fixed output rate commonly used by the
  // ESP32-audioI2S library and is also supported by PCM5102A.
  cfg.sample_rate = 48000;
  cfg.bits_per_sample = 16;
  cfg.channels = 2;

  // Larger DMA buffering helps prevent underruns/clicks.
  cfg.buffer_size = 128;
  cfg.buffer_count = 12;

  if (!i2s.begin(cfg)) {
    Serial.println("ERROR: AudioTools I2S initialization failed!");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("FNK0047 I2S initialized.");
  Serial.println("BCK=26  WS=25  DIN=22");
  Serial.println("Starting Bluetooth Classic A2DP...");

  a2dp_sink.set_discoverability(ESP_BT_GENERAL_DISCOVERABLE);
  a2dp_sink.set_connectable(true);

  // Bluetooth MUSIC RECEIVER.
  a2dp_sink.start("LOud! Com.");

  Serial.println();
  Serial.println("========================================");
  Serial.println("Bluetooth name: LOud! Com.");
  Serial.println("Bluetooth mode: Classic A2DP");
  Serial.println("Discoverable: YES");
  Serial.println("FNK0047 PCM5102A output: READY");
  Serial.println("YouTube/audio -> ESP32 -> PCM5102A");
  Serial.println("========================================");
}

void loop() {
  delay(100);
}
