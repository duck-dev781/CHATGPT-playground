/*
  RALPH - Offline ESP32 WROVER-E character
  ------------------------------------------------------------
  Hardware target:
    - ESP32 WROVER-E / Freenove-style board
    - 16x2 I2C LCD, normally 0x27
    - MPU6050, normally 0x68
    - onboard SD card via SD_MMC
    - BLE

  Ralph features:
    - first-boot BLE setup with a generated setup key
    - settings + memories + chat log on SD
    - offline response engine with templates and an optional SD response DB
    - °F ESP32 internal temperature display
    - gravity/orientation detection
    - fall when turned upside down
    - shake/dizziness reactions
    - house animation that jumps during hard shaking
    - 30-minute inactivity sleep
    - brake-light output turns off while sleeping
    - tiny movement wakes Ralph
    - animation frames live in characters.h

  BLE:
    Service UUID:        7f6c0001-9f42-4e9b-8d11-72616c706800
    Command RX UUID:     7f6c0002-9f42-4e9b-8d11-72616c706800
    Reply/notify UUID:   7f6c0003-9f42-4e9b-8d11-72616c706800

  Simple BLE commands:
    SETUP <key>
    GET
    NAME Ralph
    OWNER Chase
    HOUSE TinyHouse
    CHAT hello Ralph
    REMEMBER favorite=Minecraft
    FORGET favorite
    SLEEP
    WAKE
    STATUS
    RESETSETUP

  NOTE:
    This is an offline character/response engine, not a full Llama model.
    A real tiny language model can be added later behind the same brain
    interface. The SD card stores model/data files, but inference still
    needs RAM/CPU on the ESP32.
*/

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SD_MMC.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_system.h>
#include <math.h>
#include "characters.h"

// ----------------------------- Pins / addresses -----------------------------
static constexpr uint8_t LCD_ADDR = 0x27;
static constexpr uint8_t MPU_ADDR = 0x68;

// Pick a safe GPIO for your external brake LED if you have one.
// Set to -1 to disable the physical brake LED and keep the state internal.
static constexpr int BRAKE_LED_PIN = 25;

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ----------------------------- BLE UUIDs ------------------------------------
static const char* BLE_SERVICE_UUID = "7f6c0001-9f42-4e9b-8d11-72616c706800";
static const char* BLE_RX_UUID      = "7f6c0002-9f42-4e9b-8d11-72616c706800";
static const char* BLE_TX_UUID      = "7f6c0003-9f42-4e9b-8d11-72616c706800";

BLECharacteristic* txChar = nullptr;
bool bleConnected = false;

// ----------------------------- State ----------------------------------------
enum RalphState {
  BOOTING,
  SETUP,
  AWAKE,
  TALKING,
  DIZZY,
  FALLEN,
  SLEEPING,
  WAKING
};

RalphState state = BOOTING;

struct Settings {
  String name = "Ralph";
  String owner = "";
  String house = "Tiny House";
  String personality = "friendly";
  bool setupComplete = false;
  String setupKey = "";
};

Settings settings;

// ----------------------------- MPU ------------------------------------------
float ax = 0, ay = 0, az = 0;
float lastMagnitude = 1.0f;
float shakeScore = 0.0f;
bool mpuOK = false;

static constexpr uint32_t SLEEP_AFTER_MS = 30UL * 60UL * 1000UL;
static constexpr float MOVEMENT_WAKE_DELTA_G = 0.12f;
static constexpr float SHAKE_START_G = 0.75f;
static constexpr float HARD_SHAKE_G = 1.65f;
static constexpr float UPSIDE_DOWN_G = 0.72f;

uint32_t lastMovementMs = 0;
uint32_t lastFrameMs = 0;
uint32_t stateUntilMs = 0;
uint32_t lastTempMs = 0;
uint32_t lastLcdMs = 0;
uint32_t lastShakeMs = 0;

float espTempF = 0.0f;
bool brakeLightOn = true;
uint8_t animFrame = 0;
int8_t houseOffset = 0;

// ----------------------------- SD paths -------------------------------------
static const char* ROOT = "/RALPH";
static const char* SETTINGS_FILE = "/RALPH/SETTINGS.TXT";
static const char* MEMORY_FILE = "/RALPH/MEMORY.TXT";
static const char* CHAT_FILE = "/RALPH/CHATS.TXT";
static const char* RESPONSE_FILE = "/RALPH/AI/RESPONSES.TXT";

// ----------------------------- LCD helpers ---------------------------------
void lcdLine(uint8_t row, String text) {
  if (text.length() > 16) text = text.substring(0, 16);
  while (text.length() < 16) text += ' ';
  lcd.setCursor(0, row);
  lcd.print(text);
}

void show2(String a, String b, uint16_t holdMs = 0) {
  lcdLine(0, a);
  lcdLine(1, b);
  if (holdMs) delay(holdMs);
}

void loadCharacters() {
  for (uint8_t i = 0; i < 8; i++) {
    lcd.createChar(i, (uint8_t*)RALPH_CHARS[i]);
  }
}

// ----------------------------- SD helpers ----------------------------------
bool sdOK = false;

void ensureDirs() {
  if (!sdOK) return;
  SD_MMC.mkdir("/RALPH");
  SD_MMC.mkdir("/RALPH/AI");
  SD_MMC.mkdir("/RALPH/CHATS");
}

String readWholeFile(const char* path) {
  if (!sdOK || !SD_MMC.exists(path)) return "";
  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return "";
  String s;
  while (f.available()) s += char(f.read());
  f.close();
  return s;
}

void writeWholeFile(const char* path, const String& data) {
  if (!sdOK) return;
  File f = SD_MMC.open(path, FILE_WRITE);
  if (!f) return;
  f.print(data);
  f.close();
}

void appendFile(const char* path, const String& line) {
  if (!sdOK) return;
  File f = SD_MMC.open(path, FILE_APPEND);
  if (!f) return;
  f.println(line);
  f.close();
}

String getSetting(String key) {
  String data = readWholeFile(SETTINGS_FILE);
  int p = 0;
  while (p < data.length()) {
    int e = data.indexOf('\n', p);
    if (e < 0) e = data.length();
    String line = data.substring(p, e);
    line.trim();
    int eq = line.indexOf('=');
    if (eq > 0 && line.substring(0, eq) == key) return line.substring(eq + 1);
    p = e + 1;
  }
  return "";
}

void saveSettings() {
  if (!sdOK) return;
  String out;
  out += "name=" + settings.name + "\n";
  out += "owner=" + settings.owner + "\n";
  out += "house=" + settings.house + "\n";
  out += "personality=" + settings.personality + "\n";
  out += "setupComplete=" + String(settings.setupComplete ? 1 : 0) + "\n";
  out += "setupKey=" + settings.setupKey + "\n";
  writeWholeFile(SETTINGS_FILE, out);
}

void loadSettings() {
  if (!sdOK || !SD_MMC.exists(SETTINGS_FILE)) return;
  String v;
  v = getSetting("name"); if (v.length()) settings.name = v;
  v = getSetting("owner"); settings.owner = v;
  v = getSetting("house"); if (v.length()) settings.house = v;
  v = getSetting("personality"); if (v.length()) settings.personality = v;
  v = getSetting("setupKey"); settings.setupKey = v;
  settings.setupComplete = (getSetting("setupComplete") == "1");
}

String generateSetupKey() {
  const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
  String key;
  uint32_t seed = esp_random();
  for (int i = 0; i < 12; i++) {
    seed = seed * 1664525UL + 1013904223UL;
    key += alphabet[seed % (sizeof(alphabet) - 1)];
  }
  return key;
}

// ----------------------------- MPU6050 -------------------------------------
void mpuWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

bool mpuReadRaw(int16_t& rx, int16_t& ry, int16_t& rz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)6) != 6) return false;
  rx = (int16_t)((Wire.read() << 8) | Wire.read());
  ry = (int16_t)((Wire.read() << 8) | Wire.read());
  rz = (int16_t)((Wire.read() << 8) | Wire.read());
  return true;
}

bool initMPU() {
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x75);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)1) != 1) return false;
  uint8_t who = Wire.read();
  if (who != 0x68 && who != 0x70) return false;
  mpuWrite(0x6B, 0x00); // wake
  mpuWrite(0x1C, 0x00); // +/-2g
  return true;
}

bool updateMPU() {
  if (!mpuOK) return false;
  int16_t rx, ry, rz;
  if (!mpuReadRaw(rx, ry, rz)) return false;

  ax = rx / 16384.0f;
  ay = ry / 16384.0f;
  az = rz / 16384.0f;

  float mag = sqrtf(ax * ax + ay * ay + az * az);
  float delta = fabsf(mag - lastMagnitude);
  lastMagnitude = mag;

  // Exponential-ish shake score: quick changes accumulate, then decay.
  shakeScore *= 0.88f;
  if (delta > SHAKE_START_G) shakeScore += delta;

  if (delta > MOVEMENT_WAKE_DELTA_G || fabsf(mag - 1.0f) > 0.10f) {
    lastMovementMs = millis();
    if (state == SLEEPING) {
      state = WAKING;
      stateUntilMs = millis() + 1800;
      brakeLightOn = true;
    }
  }

  // Gravity direction. Any axis strongly negative is treated as inverted.
  bool upsideDown = (az < -UPSIDE_DOWN_G || ay < -UPSIDE_DOWN_G || ax < -UPSIDE_DOWN_G);

  if (state != SLEEPING && state != SETUP) {
    if (upsideDown && mag > 0.75f && mag < 1.25f) {
      state = FALLEN;
      stateUntilMs = millis() + 3000;
    } else if (shakeScore > HARD_SHAKE_G) {
      state = DIZZY;
      stateUntilMs = millis() + 5000;
      lastShakeMs = millis();
    } else if (shakeScore > SHAKE_START_G) {
      lastShakeMs = millis();
    }
  }

  return true;
}

// ----------------------------- Temperature ---------------------------------
void updateTemperature() {
  if (millis() - lastTempMs < 2000) return;
  lastTempMs = millis();

  // Arduino-ESP32 exposes the internal sensor through temperatureRead().
  // It is chip temperature, not room temperature.
  float c = temperatureRead();
  espTempF = c * 9.0f / 5.0f + 32.0f;
}

// ----------------------------- Brake light ---------------------------------
void setBrakeLight(bool on) {
  brakeLightOn = on;
  if (BRAKE_LED_PIN >= 0) digitalWrite(BRAKE_LED_PIN, on ? HIGH : LOW);
}

// ----------------------------- BLE ------------------------------------------
class RalphServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {
    bleConnected = true;
  }

  void onDisconnect(BLEServer* server) override {
    bleConnected = false;
    server->getAdvertising()->start();
  }
};

void bleReply(const String& msg) {
  if (!txChar) return;
  txChar->setValue(msg.c_str());
  txChar->notify();
}

bool checkSetupKey(const String& key) {
  return settings.setupKey.length() && key == settings.setupKey;
}

// ----------------------------- Offline brain -------------------------------
String lower(String s) {
  s.toLowerCase();
  return s;
}

String randomChoice(const String* arr, size_t count) {
  if (!count) return "";
  return arr[esp_random() % count];
}

String memoryValue(String key) {
  if (!sdOK || !SD_MMC.exists(MEMORY_FILE)) return "";
  String data = readWholeFile(MEMORY_FILE);
  String wanted = key + "=";
  int p = 0;
  while (p < data.length()) {
    int e = data.indexOf('\n', p);
    if (e < 0) e = data.length();
    String line = data.substring(p, e);
    line.trim();
    if (line.startsWith(wanted)) return line.substring(wanted.length());
    p = e + 1;
  }
  return "";
}

void rememberValue(String key, String value) {
  if (!sdOK) return;
  String data = readWholeFile(MEMORY_FILE);
  String wanted = key + "=";
  int p = 0;
  bool replaced = false;
  String out;
  while (p < data.length()) {
    int e = data.indexOf('\n', p);
    if (e < 0) e = data.length();
    String line = data.substring(p, e);
    line.trim();
    if (line.length() && line.startsWith(wanted)) {
      out += key + "=" + value + "\n";
      replaced = true;
    } else if (line.length()) {
      out += line + "\n";
    }
    p = e + 1;
  }
  if (!replaced) out += key + "=" + value + "\n";
  writeWholeFile(MEMORY_FILE, out);
}

String templateResponse(String prompt) {
  String p = lower(prompt);

  if (p.indexOf("temperature") >= 0 || p.indexOf("temp") >= 0) {
    return "My chip temp is " + String(espTempF, 1) + "F.";
  }

  if (p.indexOf("hello") >= 0 || p == "hi" || p.indexOf("hey") >= 0) {
    const String r[] = {
      "Hi! I'm Ralph!",
      "Heyyy! Ralph here.",
      "Oh! You came back!",
      "HELLOOO!",
      "Hi hi!"
    };
    return randomChoice(r, 5);
  }

  if (p.indexOf("who are you") >= 0 || p.indexOf("your name") >= 0) {
    return "I'm Ralph. This is my tiny house!";
  }

  if (p.indexOf("house") >= 0 || p.indexOf("home") >= 0) {
    return "My house is " + settings.house + ". I like it here.";
  }

  if (p.indexOf("sleep") >= 0) {
    state = SLEEPING;
    setBrakeLight(false);
    return "Okay... sleepy time...";
  }

  if (p.indexOf("joke") >= 0) {
    const String r[] = {
      "Why did the ESP32 nap? Too many interrupts.",
      "I told my house a joke. It had no windows to laugh through.",
      "I have a byte-sized sense of humor."
    };
    return randomChoice(r, 3);
  }

  if (p.indexOf("favorite") >= 0 && p.indexOf("game") >= 0) {
    String v = memoryValue("favorite_game");
    if (v.length()) return "You said your favorite game is " + v + ".";
    return "I don't know your favorite game yet!";
  }

  if (p.indexOf("dizzy") >= 0) {
    return "Please stop spinning me around!";
  }

  // Try the optional SD response database.
  if (sdOK && SD_MMC.exists(RESPONSE_FILE)) {
    File f = SD_MMC.open(RESPONSE_FILE, FILE_READ);
    if (f) {
      while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        int sep = line.indexOf('|');
        if (sep > 0) {
          String trigger = line.substring(0, sep);
          String response = line.substring(sep + 1);
          if (p.indexOf(lower(trigger)) >= 0) {
            f.close();
            return response;
          }
        }
      }
      f.close();
    }
  }

  const String fallback[] = {
    "Hmm... tell me more.",
    "My tiny brain is thinking...",
    "I don't know that one yet.",
    "Interesting!",
    "Can you say that another way?",
    "I heard you!",
    "My offline brain needs more training for that."
  };
  return randomChoice(fallback, 7);
}

String chatWithRalph(String prompt) {
  String answer = templateResponse(prompt);
  appendFile(CHAT_FILE, "YOU: " + prompt);
  appendFile(CHAT_FILE, "RALPH: " + answer);
  state = TALKING;
  stateUntilMs = millis() + 2500;
  return answer;
}

// ----------------------------- BLE command parser ---------------------------
void processCommand(String cmd) {
  cmd.trim();
  if (!cmd.length()) return;

  if (!settings.setupComplete) {
    if (cmd.startsWith("SETUP ")) {
      String key = cmd.substring(6);
      key.trim();
      if (checkSetupKey(key)) {
        settings.setupComplete = true;
        saveSettings();
        state = AWAKE;
        setBrakeLight(true);
        bleReply("SETUP OK - Ralph is ready.");
        show2("SETUP COMPLETE", "Hi! I'm Ralph", 1200);
      } else {
        bleReply("BAD KEY");
      }
      return;
    }

    if (cmd == "KEY") {
      bleReply("SETUP KEY: " + settings.setupKey);
      return;
    }

    bleReply("Ralph needs setup. Use SETUP <key>.");
    return;
  }

  if (cmd == "GET") {
    bleReply("NAME=" + settings.name + ", HOUSE=" + settings.house);
    return;
  }

  if (cmd.startsWith("NAME ")) {
    settings.name = cmd.substring(5); settings.name.trim();
    saveSettings(); bleReply("NAME SAVED"); return;
  }

  if (cmd.startsWith("OWNER ")) {
    settings.owner = cmd.substring(6); settings.owner.trim();
    saveSettings(); bleReply("OWNER SAVED"); return;
  }

  if (cmd.startsWith("HOUSE ")) {
    settings.house = cmd.substring(6); settings.house.trim();
    saveSettings(); bleReply("HOUSE SAVED"); return;
  }

  if (cmd.startsWith("PERSONALITY ")) {
    settings.personality = cmd.substring(12); settings.personality.trim();
    saveSettings(); bleReply("PERSONALITY SAVED"); return;
  }

  if (cmd.startsWith("REMEMBER ")) {
    String m = cmd.substring(9);
    int eq = m.indexOf('=');
    if (eq > 0) {
      rememberValue(m.substring(0, eq), m.substring(eq + 1));
      bleReply("MEMORY SAVED");
    } else {
      bleReply("USE REMEMBER key=value");
    }
    return;
  }

  if (cmd.startsWith("FORGET ")) {
    rememberValue(cmd.substring(7), "");
    bleReply("MEMORY CLEARED");
    return;
  }

  if (cmd == "SLEEP") {
    state = SLEEPING;
    setBrakeLight(false);
    bleReply("Goodnight.");
    return;
  }

  if (cmd == "WAKE") {
    state = WAKING;
    stateUntilMs = millis() + 1800;
    setBrakeLight(true);
    bleReply("I'm awake!");
    return;
  }

  if (cmd == "STATUS") {
    String s = "state=" + String((int)state);
    s += " tempF=" + String(espTempF, 1);
    s += " mpu=" + String(mpuOK ? "ok" : "missing");
    s += " sd=" + String(sdOK ? "ok" : "missing");
    s += " brake=" + String(brakeLightOn ? "on" : "off");
    bleReply(s);
    return;
  }

  if (cmd == "RESETSETUP") {
    settings.setupComplete = false;
    saveSettings();
    state = SETUP;
    setBrakeLight(false);
    bleReply("SETUP RESET");
    return;
  }

  if (cmd.startsWith("CHAT ")) {
    String prompt = cmd.substring(5);
    bleReply(chatWithRalph(prompt));
    return;
  }

  // Treat any unrecognized BLE text as chat.
  bleReply(chatWithRalph(cmd));
}

class RalphRxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    std::string value = characteristic->getValue();
    if (!value.empty()) processCommand(String(value.c_str()));
  }
};

void initBLE() {
  BLEDevice::init("Ralph-ESP32");
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new RalphServerCallbacks());

  BLEService* service = server->createService(BLE_SERVICE_UUID);

  BLECharacteristic* rx = service->createCharacteristic(
    BLE_RX_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  rx->setCallbacks(new RalphRxCallbacks());

  txChar = service->createCharacteristic(
    BLE_TX_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  txChar->addDescriptor(new BLE2902());

  service->start();
  server->getAdvertising()->start();
}

// ----------------------------- Animation ------------------------------------
void drawCharacter(uint8_t frame, int8_t x, int8_t y = 0) {
  if (x < 0) x = 0;
  if (x > 15) x = 15;
  lcd.setCursor(x, y);
  lcd.write((uint8_t)(frame % 8));
}

void drawNormal() {
  lcd.clear();
  // Face
  drawCharacter(animFrame % 4, 1, 0);
  drawCharacter((animFrame + 1) % 4, 2, 0);

  // House + little Ralph
  int x = 9 + houseOffset;
  if (x < 0) x = 0;
  if (x > 14) x = 14;
  lcd.setCursor(x, 1);
  lcd.write((uint8_t)4);
  lcd.setCursor(x + 1, 1);
  lcd.write((uint8_t)5);

  lcd.setCursor(5, 0);
  String t = String(espTempF, 0) + "F";
  lcd.print(t);
}

void drawSetup() {
  lcd.clear();
  lcdLine(0, "Ralph setup");
  String key = settings.setupKey;
  lcdLine(1, key);
}

void drawSleeping() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("      zZz");
  lcd.setCursor(4, 1);
  lcd.write((uint8_t)6);
  lcd.print("  ");
  lcd.write((uint8_t)7);
}

void drawDizzy() {
  lcd.clear();
  lcdLine(0, "@_@  DIZZY!");
  lcd.setCursor(4, 1);
  lcd.write((uint8_t)2);
  lcd.print("   ");
  lcd.write((uint8_t)3);
}

void drawFallen() {
  lcd.clear();
  lcdLine(0, "Ralph fell!");
  lcd.setCursor(3, 1);
  lcd.write((uint8_t)1);
  lcd.write((uint8_t)0);
  lcd.print("  ");
  lcd.write((uint8_t)4);
}

void drawWaking() {
  lcd.clear();
  lcdLine(0, "Huh...?");
  lcd.setCursor(6, 1);
  lcd.write((uint8_t)(animFrame % 4));
}

void animate() {
  uint32_t now = millis();

  if (!settings.setupComplete || state == SETUP) {
    if (now - lastLcdMs > 500) {
      lastLcdMs = now;
      drawSetup();
    }
    return;
  }

  if (state == SLEEPING) {
    if (now - lastLcdMs > 1000) {
      lastLcdMs = now;
      drawSleeping();
    }
    return;
  }

  if (now - lastFrameMs < 350) return;
  lastFrameMs = now;
  animFrame++;

  if (state == DIZZY) {
    houseOffset = (animFrame % 2) ? -1 : 1;
    drawDizzy();
  } else if (state == FALLEN) {
    houseOffset = 0;
    drawFallen();
  } else if (state == WAKING) {
    drawWaking();
  } else {
    // Hard shake makes the house jump left/right.
    if (shakeScore > SHAKE_START_G && millis() - lastShakeMs < 1000) {
      houseOffset = (animFrame % 3) - 1;
    } else {
      houseOffset = 0;
    }
    drawNormal();
  }

  if ((state == DIZZY || state == FALLEN || state == WAKING || state == TALKING) &&
      now >= stateUntilMs) {
    state = AWAKE;
    setBrakeLight(true);
  }
}

// ----------------------------- Setup ----------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(BRAKE_LED_PIN, OUTPUT);
  setBrakeLight(false);

  lcd.init();
  lcd.backlight();
  loadCharacters();

  show2("RALPH", "booting...", 800);

  // SD_MMC 1-bit mode is a common fit for ESP32 WROVER SD wiring.
  sdOK = SD_MMC.begin("/sdcard", true);
  if (sdOK) {
    ensureDirs();
    loadSettings();
  }

  if (!settings.setupKey.length()) {
    settings.setupKey = generateSetupKey();
    saveSettings();
  }

  mpuOK = initMPU();
  lastMovementMs = millis();

  initBLE();
  updateTemperature();

  if (!settings.setupComplete) {
    state = SETUP;
    setBrakeLight(false);
    show2("SETUP KEY", settings.setupKey, 1500);
    bleReply("Ralph setup key: " + settings.setupKey);
  } else {
    state = AWAKE;
    setBrakeLight(true);
    show2("Hi! I'm Ralph", settings.house, 1200);
  }
}

void loop() {
  updateMPU();
  updateTemperature();

  // 30-minute no-movement sleep timer.
  if (settings.setupComplete &&
      state != SLEEPING &&
      millis() - lastMovementMs >= SLEEP_AFTER_MS) {
    state = SLEEPING;
    setBrakeLight(false);
  }

  // Recover from a fall once the board is returned upright.
  if (state == FALLEN && az > 0.65f && fabsf(ax) < 0.65f && fabsf(ay) < 0.65f) {
    state = WAKING;
    stateUntilMs = millis() + 1800;
    setBrakeLight(true);
  }

  // Shake score naturally decays.
  if (millis() - lastShakeMs > 1000) shakeScore *= 0.92f;

  animate();
  delay(5);
}
