# Firmware Games

Every directory under `games/` is a game compiled into the ESP32 firmware.

## Game layout

```text
games/
  game_id/
    main.cpp
    0.h
    1.h
    ...
```

`main.cpp` must define a `GameDefinition` and register it with the game manager. Header files can contain game assets/data.

The ESP-IDF build automatically discovers every `games/*/main.cpp` file, so adding a new game directory is enough to include it in the firmware build.

Games are firmware code, not SD-card programs. The SD card is reserved for user media/settings and future external game data.
