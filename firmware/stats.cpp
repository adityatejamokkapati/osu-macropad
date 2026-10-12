#include "stats.h"
#include <LittleFS.h>

namespace {
constexpr const char* kPath = "/macropad.bin";
constexpr uint32_t kMagic = 0x4F535550;  // "OSUP"
constexpr uint32_t kSaveMinIntervalMs = 5000;

struct Persist {
  uint32_t magic;
  uint8_t layer;
  uint8_t volume;
  uint8_t muted;
  uint8_t reserved;
  uint32_t totals[5];
};
}  // namespace

void Stats::begin(uint8_t& activeLayer) {
  if (!LittleFS.begin()) {
#if SERIAL_DEBUG
    Serial.println(F("[stats] LittleFS mount failed, counters start at 0"));
#endif
    return;
  }
  load(activeLayer);
}

void Stats::load(uint8_t& activeLayer) {
  File f = LittleFS.open(kPath, "r");
  if (!f) return;
  Persist p{};
  if (f.read((uint8_t*)&p, sizeof(p)) == sizeof(p) && p.magic == kMagic) {
    if (p.layer < NUM_LAYERS) activeLayer = p.layer;
    volume_ = p.volume > 100 ? 50 : p.volume;
    muted_ = p.muted != 0;
    memcpy(totals_, p.totals, sizeof(totals_));
#if SERIAL_DEBUG
    Serial.printf("[stats] loaded layer=%u total=%lu\n", activeLayer,
                  (unsigned long)grandTotal());
#endif
  }
  f.close();
}

void Stats::recordPress(uint8_t keyIndex) {
  if (keyIndex < NUM_KEYS) {
    totals_[keyIndex]++;
    if (totals_[keyIndex] == 0) totals_[keyIndex]--;  // saturate, never wrap
  }
  pressTimes_[pressHead_] = millis();
  pressHead_ = (uint8_t)((pressHead_ + 1) % kCpsBuf);
}

uint16_t Stats::cps() {
  uint32_t now = millis();
  uint16_t n = 0;
  for (uint8_t i = 0; i < kCpsBuf; i++) {
    if (pressTimes_[i] != 0 && (now - pressTimes_[i]) < CPS_WINDOW_MS) n++;
  }
  return n;
}

uint32_t Stats::total(uint8_t keyIndex) const {
  return keyIndex < NUM_KEYS ? totals_[keyIndex] : 0;
}

uint32_t Stats::grandTotal() const {
  uint32_t s = 0;
  for (uint8_t i = 0; i < NUM_KEYS; i++) s += totals_[i];
  return s;
}

void Stats::setVolume(uint8_t vol, bool muted) {
  volume_ = vol > 100 ? 100 : vol;
  muted_ = muted;
}

void Stats::save(uint8_t activeLayer) {
  uint32_t now = millis();
  if (now - lastSaveMs_ < kSaveMinIntervalMs && lastSaveMs_ != 0) return;
  lastSaveMs_ = now;
  write(activeLayer);
}

void Stats::saveNow(uint8_t activeLayer) {
  lastSaveMs_ = millis();
  write(activeLayer);
}

void Stats::write(uint8_t activeLayer) {
  Persist p{};
  p.magic = kMagic;
  p.layer = activeLayer;
  p.volume = volume_;
  p.muted = muted_ ? 1 : 0;
  memcpy(p.totals, totals_, sizeof(totals_));
  File f = LittleFS.open(kPath, "w");
  if (!f) return;
  f.write((const uint8_t*)&p, sizeof(p));
  f.close();
}
