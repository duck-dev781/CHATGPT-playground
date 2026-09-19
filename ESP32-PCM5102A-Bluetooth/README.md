# Freenove ESP32 PCM5102A Bluetooth Speaker

This project turns the ESP32-WROVER into a Bluetooth Classic A2DP music speaker/receiver.

Audio path:

**Phone / Chromebook -> Bluetooth A2DP -> ESP32 -> PCM5102A -> amplifier/speaker**

That means audio from apps such as YouTube and YouTube Music can be sent to the ESP32 after connecting to **LOud! Com.**

## Arduino IDE

Keep **ESP32 Arduino Core 3.3.12**.

Install these two libraries from the Arduino Library Manager:

1. **ESP32-A2DP** by Phil Schatzmann
2. **AudioTools** (Arduino Audio Tools) by Phil Schatzmann

This version uses AudioTools' I2SStream, which is the version-independent audio output method documented by ESP32-A2DP for current ESP32 Arduino releases.

**Do not install BLE libraries for the music connection.** This project uses Bluetooth Classic A2DP.

## PCM5102A wiring

| PCM5102A | ESP32 |
|---|---|
| BCK | GPIO 26 |
| LCK / LRCK / WS | GPIO 25 |
| DIN | GPIO 22 |
| SCK | Leave unconnected |
| VCC | 5V |
| GND | GND |

The speaker must be connected to the appropriate amplified output. Do not connect a speaker directly between L and R.

## Upload

1. Install **ESP32-A2DP**.
2. Install **AudioTools**.
3. Open ESP32_PCM5102A_Bluetooth.ino.
4. Select your ESP32-WROVER board.
5. Keep ESP32 Arduino Core **3.3.12**.
6. Upload.
7. Open Serial Monitor at **115200 baud**.
8. Look for **LOud! Com.** in the Bluetooth device list.
9. Connect to it.
10. Play YouTube or other audio on the connected device.

## Important

This sketch is specifically a **Bluetooth A2DP music receiver**. It is not a BLE speaker/control device.

The project was changed from the previous direct ESP_I2S.h output implementation to the AudioTools I2SStream implementation, while keeping the Freenove PCM5102A pinout of GPIO 26 / 25 / 22.
