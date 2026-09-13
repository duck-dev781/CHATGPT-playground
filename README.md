# ESP32 Music Player 2.0

ESP-IDF project for the Freenove FNK0047 / ESP32-WROVER-E music player.

## Current foundation
- 1602 I2C LCD status display
- Wi-Fi station support
- Six-character per-device connection code in firmware configuration
- Optional microSD detection/mounting foundation
- Flash-backed factory defaults
- SD settings/backup layout foundation
- No physical buttons or IR receiver

## Build in Codespaces

```bash
. $HOME/esp/esp-idf/export.sh
idf.py set-target esp32
idf.py build
```

The firmware binaries are produced in `build/`.

Hardware-specific audio-module pins are intentionally left configurable until the exact audio module wiring is confirmed.
