# Freenove ESP32 FNK0047 PCM5102A Bluetooth Speaker

This project turns the ESP32-WROVER from the FNK0047 kit into a Bluetooth Classic A2DP music receiver and sends the audio to the FNK0047 Audio Converter & Amplifier (PCM5102A).

Audio path:

**Phone / Chromebook -> Bluetooth A2DP -> ESP32 -> I2S -> FNK0047 PCM5102A -> amplifier/speaker**

Audio from apps such as YouTube and YouTube Music can be sent to the ESP32 after connecting to **LOud! Com.**

## Arduino IDE

Keep **ESP32 Arduino Core 3.3.12**.

Install these libraries:

1. **ESP32-A2DP** by Phil Schatzmann
2. **AudioTools** by Phil Schatzmann
3. **ESP32-audioI2S** / **AudioI2S** from the Freenove FNK0047 kit

For the Freenove library, the FNK0047 documentation calls the ZIP library **ESP32-audioI2S.zip** and uses it with `#include "Audio.h"` for the Audio Converter & Amplifier/PCM5102A projects.

The live Bluetooth audio stream is still handled by ESP32-A2DP. AudioTools provides the Arduino Core 3.x-compatible I2S output path. The Freenove AudioI2S library is included because this is the FNK0047 audio library for the same PCM5102A hardware; it is not used as an MP3 decoder for the live A2DP stream.

**Do not install BLE libraries for the music connection.** This project uses Bluetooth Classic A2DP.

## PCM5102A wiring

| FNK0047 Audio Converter & Amplifier | ESP32-WROVER |
|---|---|
| BCK | GPIO 26 |
| LCK / LRCK / WS | GPIO 25 |
| DIN | GPIO 22 |
| SCK | Leave unconnected |
| VCC | 5V |
| GND | GND |

The speaker connects to one complete amplified channel pair: **L+ / L-** or **R+ / R-**. Do not connect a speaker between L and R.

## Audio output

The sketch uses:

- 16-bit stereo
- 48 kHz I2S output
- GPIO 26 BCK
- GPIO 25 WS/LCK
- GPIO 22 DIN
- larger I2S buffering to reduce underruns/clicks

## Upload

1. Install **ESP32-A2DP**.
2. Install **AudioTools**.
3. Install the **Freenove ESP32-audioI2S** library from the FNK0047 kit ZIP.
4. Open `ESP32_PCM5102A_Bluetooth.ino`.
5. Select your ESP32-WROVER board.
6. Keep ESP32 Arduino Core **3.3.12**.
7. Upload.
8. Open Serial Monitor at **115200 baud**.
9. Look for **LOud! Com.** in the Bluetooth device list.
10. Connect to it.
11. Play YouTube or other audio.

## Important

This sketch is a **Bluetooth Classic A2DP music receiver**, not a BLE speaker/control device.

The project targets the actual **Freenove FNK0047 Audio Converter & Amplifier / PCM5102A** hardware and includes the Freenove `Audio.h` dependency in addition to the modern Core 3.x A2DP/I2S output stack.
