# Ralph — Offline ESP32 WROVER-E Character

Ralph is a standalone little character designed for an ESP32 WROVER-E with a 16x2 I2C LCD, MPU6050, SD card and BLE.

The goal is to make Ralph feel like a physical creature instead of just a display demo.

## Hardware

- ESP32 WROVER-E
- 16x2 I2C LCD (default address 0x27)
- MPU6050 (default address 0x68)
- SD card (the sketch uses ESP32 SD_MMC in 1-bit mode)
- BLE built into the ESP32
- Optional external LED/brake light on GPIO 25

The brake-light pin is defined at the top of `Ralph.ino`. If your Freenove board uses another pin, change it before uploading.

## Ralph's behavior

### Gravity and movement

The MPU6050 is used as Ralph's movement and gravity sensor.

- normal orientation: Ralph is awake
- tilt: Ralph can react to the movement
- upside down: Ralph falls
- hard shaking: Ralph becomes dizzy
- extreme shaking: the house animation jumps around too
- movement while sleeping: Ralph wakes

The firmware uses acceleration changes instead of a simple "any sensor value changed" test so normal MPU noise does not constantly wake him.

### Automatic sleep

After 30 minutes without meaningful movement:

1. Ralph enters SLEEPING state.
2. The brake-light output turns off.
3. LCD switches to a sleeping animation.
4. A small movement wakes him.

The MPU remains active while sleeping because it is what detects the movement that wakes Ralph.

### Temperature

The LCD displays the ESP32 internal chip temperature in Fahrenheit.

This is **not an ambient room-temperature sensor**. It is the ESP32's internal temperature reading.

## First-time setup

On the first boot, Ralph creates a random setup key and stores it on the SD card:

`/RALPH/SETTINGS.TXT`

The LCD shows the key.

Connect to BLE device:

`Ralph-ESP32`

Then send:

`SETUP <key>`

Example:

`SETUP RXDFtge54ksde`

After successful setup, Ralph switches to normal operation.

The setup key is generated automatically; the example above is only an example.

## BLE commands

The RX characteristic accepts text commands.

### Setup

`KEY`

Shows the setup key.

`SETUP <key>`

Completes first-time setup.

### Character settings

`NAME Ralph`

`OWNER Chase`

`HOUSE Tiny House`

`PERSONALITY friendly`

### Chat

`CHAT hello Ralph`

You can also send ordinary text without `CHAT `; Ralph treats it as chat.

### Memory

`REMEMBER favorite_game=Minecraft`

`FORGET favorite_game`

Memory is stored in:

`/RALPH/MEMORY.TXT`

### Control

`SLEEP`

`WAKE`

`STATUS`

`RESETSETUP`

## SD-card layout

The firmware creates:

```
/RALPH/
  SETTINGS.TXT
  MEMORY.TXT
  CHATS.TXT
  /AI/
    RESPONSES.TXT
  /CHATS/
```

### RESPONSES.TXT

You can add simple offline response rules:

```
pizza|Pizza sounds good!
minecraft|Minecraft time!
hello|Hello there!
```

The left side is a trigger and the right side is the response.

This is intentionally simple so the ESP32 can search it without needing a large framework.

## About the "AI"

A 32 GB SD card can hold a huge amount of text, but SD capacity does not automatically give the ESP32 more RAM or CPU.

Ralph therefore starts with an **offline character brain**:

- rules
- response templates
- random responses
- memory
- conversation history
- SD response database

The project is structured so a future tiny/quantized language model can be added behind the same chat interface.

A full desktop-size Llama model is not a realistic target for this ESP32 simply because of inference RAM and compute requirements, even if its model file fits on the SD card.

## LCD animations

The eight HD44780 custom-character slots are defined in:

`characters.h`

That file contains Ralph's reusable 5x8 pixel frames and the house pieces.

More animation logic can be added in `animate()` without changing the storage/communication system.

## Important hardware notes

### LCD

Default:

- SDA/SCL from the ESP32 I2C pins
- address 0x27

If your LCD uses another address, change `LCD_ADDR`.

### MPU6050

Default I2C address:

`0x68`

If AD0 is high, the address may be `0x69`; the current sketch expects 0x68 and should be adjusted for 0x69 if needed.

### SD

This sketch uses:

`SD_MMC.begin("/sdcard", true)`

The `true` enables 1-bit mode. This is useful for boards where the SD wiring conflicts with other ESP32 resources, but the exact wiring on a particular WROVER carrier can differ.

## Current version

This is the first Ralph firmware foundation. It intentionally prioritizes a stable architecture:

```
                RALPH
                  |
       +----------+----------+
       |          |          |
      LCD        MPU        BLE
       |          |          |
       +----------+----------+
                  |
               ESP32
                  |
             +----+----+
             |         |
           PSRAM      SD
             |         |
          runtime   memory/data
```

The next logical expansion is a much larger SD response database, richer LCD animation states, house persistence, and an embedded tiny-model backend if a model that fits the WROVER's actual memory/compute budget is selected.
