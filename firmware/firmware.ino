// osu-macropad — Nano RP2040 Connect (arduino-pico + TinyUSB)
// 5 keys + EC11 + SSD1306. 1 ms scan, NKRO HID, 3 layers.
//
// Board: "Arduino Nano RP2040 Connect" (rp2040:rp2040, USB Stack: TinyUSB)

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "config.h"
#include "keymap.h"
#include "hid_manager.h"
#include "encoder.h"
#include "display.h"
#include "stats.h"

namespace {
HidManager hid;
Encoder enc;
Display ui;
Stats stats;

uint8_t activeLayer = 0;
bool keyStable[NUM_KEYS] = {false};   // debounced level (true = pressed)
bool keyLastRaw[NUM_KEYS] = {false};
uint32_t keyLastChangeMs[NUM_KEYS] = {0};
bool uiPressed[NUM_KEYS] = {false};   // for OLED highlight (== keyStable)

uint8_t volume = 50;  // display estimate, 0..100
bool muted = false;

uint32_t lastScanUs = 0;
uint32_t lastCpsLogMs = 0;
uint32_t lastPeriodicSaveMs = 0;
bool wasHidReady = false;

void pressAction(const Action& a);
void releaseAction(const Action& a);
void pulseAction(const Action& a);
void resyncHid();
void nextLayer();
}  // namespace

void setup() {
  Serial.begin(115200);
  // On native USB, wait briefly for monitor without blocking boot.
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0) < 1500) delay(10);

  TinyUSBDevice.setManufacturerDescriptor("osu!pad");
  TinyUSBDevice.setProductDescriptor("osu-macropad");
  TinyUSBDevice.setSerialDescriptor("OSUPAD001");

  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    pinMode(KEY_PINS[i], INPUT_PULLUP);
    bool raw = digitalRead(KEY_PINS[i]) == LOW;
    keyLastRaw[i] = raw;
    keyStable[i] = raw;
    keyLastChangeMs[i] = millis();
  }

  stats.begin(activeLayer);
  enc.begin();
  hid.begin();
  hid.releaseAllKeys();

  if (ui.begin()) ui.splash();
  delay(900);  // let splash show + USB enumerate

#if SERIAL_DEBUG
  Serial.printf("[boot] osu-macropad layer=%u keys D0/D6/D7/D8/D5 enc D2/D3/D4 oled 0x3C\n",
                activeLayer);
#endif
}

void loop() {
  uint32_t nowUs = micros();
  if ((nowUs - lastScanUs) < SCAN_INTERVAL_US) {
    hid.task();
    yield();
    return;
  }
  lastScanUs = nowUs;
  uint32_t nowMs = millis();

  // ---- USB re-sync after re-plug ----
  bool ready = hid.ready();
  if (ready && !wasHidReady) resyncHid();
  wasHidReady = ready;

  // ---- Key scan + debounce ----
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    bool raw = digitalRead(KEY_PINS[i]) == LOW;  // active-LOW
    if (raw != keyLastRaw[i]) {
      keyLastRaw[i] = raw;
      keyLastChangeMs[i] = nowMs;
      continue;
    }
    if ((nowMs - keyLastChangeMs[i]) < DEBOUNCE_MS) continue;
    if (raw == keyStable[i]) continue;

    keyStable[i] = raw;
    uiPressed[i] = raw;
    const Action& a = LAYERS[activeLayer].keys[i];

    if (raw) {
      stats.recordPress(i);
      if (ready) pressAction(a);
#if SERIAL_DEBUG
      Serial.printf("[key] K%u press layer=%u action=%s\n", i + 1, activeLayer, a.label);
#endif
    } else {
      if (ready) releaseAction(a);
    }
  }

  // ---- Encoder ----
  enc.poll();
  int8_t steps = enc.consumeSteps();
  while (steps != 0) {
    const Action& a = (steps > 0) ? LAYERS[activeLayer].encCW
                                  : LAYERS[activeLayer].encCCW;
    if (ready) pulseAction(a);
    if (a.type == ActionType::Consumer) {
      if (a.consumer == CONS_VOL_UP) { volume = volume >= 96 ? 100 : (uint8_t)(volume + 4); muted = false; }
      if (a.consumer == CONS_VOL_DOWN) { volume = volume <= 4 ? 0 : (uint8_t)(volume - 4); muted = false; }
      if (a.consumer == CONS_MUTE) muted = !muted;
    }
    stats.setVolume(volume, muted);
    steps += (steps > 0) ? -1 : 1;
  }

  if (enc.consumeShortPress()) {
    const Action& a = LAYERS[activeLayer].encShortPress;
    if (ready) pulseAction(a);
    if (a.consumer == CONS_MUTE) muted = !muted;
    stats.setVolume(volume, muted);
#if SERIAL_DEBUG
    Serial.printf("[enc] short-press -> %s (muted=%d)\n", a.label, muted);
#endif
  }
  if (enc.consumeLongPress()) {
    nextLayer();
  }

  // ---- Display (throttled internally to DISPLAY_FPS) ----
  ui.render(activeLayer, uiPressed, stats.cps(), stats.grandTotal(), volume, muted);

  // ---- Housekeeping ----
  if (nowMs - lastPeriodicSaveMs > 30000) {
    lastPeriodicSaveMs = nowMs;
    stats.setVolume(volume, muted);
    stats.save(activeLayer);
  }
#if SERIAL_DEBUG
  if (nowMs - lastCpsLogMs > 2000) {
    lastCpsLogMs = nowMs;
    Serial.printf("[stat] layer=%u cps=%u total=%lu vol=%u%s hid=%d\n",
                  activeLayer, stats.cps(), (unsigned long)stats.grandTotal(),
                  volume, muted ? " MUTED" : "", ready ? 1 : 0);
  }
#endif

  hid.task();
  yield();
}

namespace {

void pressAction(const Action& a) {
  if (a.type == ActionType::Key) {
    hid.setKey(a.key, a.mods, true);
  } else if (a.type == ActionType::Consumer) {
    hid.sendConsumer(a.consumer);
  }
}

void releaseAction(const Action& a) {
  if (a.type == ActionType::Key) {
    hid.setKey(a.key, a.mods, false);
  }
  // Consumer is a momentary pulse — nothing to release.
}

void pulseAction(const Action& a) {
  // Momentary tap: press then immediately release (for keys),
  // or a single consumer frame.
  pressAction(a);
  releaseAction(a);
}

void resyncHid() {
  hid.releaseAllKeys();
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    if (keyStable[i]) pressAction(LAYERS[activeLayer].keys[i]);
  }
#if SERIAL_DEBUG
  Serial.println(F("[usb] re-mounted, HID resynced"));
#endif
}

void nextLayer() {
  hid.releaseAllKeys();  // avoid stuck keys across layers
  activeLayer = (uint8_t)((activeLayer + 1) % NUM_LAYERS);
  stats.setVolume(volume, muted);
  stats.saveNow(activeLayer);
  ui.toast(LAYERS[activeLayer].name, "layer active");
#if SERIAL_DEBUG
  Serial.printf("[layer] -> %u (%s)\n", activeLayer, LAYERS[activeLayer].name);
#endif
}

}  // namespace
