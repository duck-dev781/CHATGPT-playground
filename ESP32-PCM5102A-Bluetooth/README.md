# Freenove ESP32 PCM5102A Bluetooth Speaker

Working target: **ESP32 Arduino Core 3.x** (tested design for 3.3.12).

This replaces Freenove's older Chapter 29 sketch, which uses the retired legacy I2S API.

## Wiring

| PCM5102A | ESP32 |
|---|---|
| BCK | GPIO 26 |
| LCK / LRCK / WS | GPIO 25 |
| DIN | GPIO 22 |
| SCK | Leave unconnected |
| VCC | 5V |
| GND | GND |

Connect the speaker to **L+ / L-** or **R+ / R-** on the audio amplifier/module. Do not connect a speaker between L and R.

## Arduino IDE

1. Keep **ESP32 Arduino Core 3.3.12**.
2. Install the **ESP32-A2DP** library by Phil Schatzmann from the Arduino Library Manager.
3. Open `ESP32_PCM5102A_Bluetooth.ino`.
4. Select the correct ESP32 board.
5. Upload.
6. Open Serial Monitor at **115200 baud**.
7. Pair your phone with **ESP32 PCM5102A**.
8. Play music.

## Why this version is different

Freenove's repository Chapter 29 sketch currently uses the old `driver/i2s.h` API. Arduino-ESP32 3.x uses the newer `ESP_I2S.h` / `I2SClass` API.

This project uses:

- `ESP_I2S.h`
- `I2SClass`
- `BluetoothA2DPSink`

No legacy `i2s_set_clk()` or `i2s_write()` calls are used.
