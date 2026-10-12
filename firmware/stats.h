#pragma once
// Key press counters + CPS (clicks per second) + LittleFS persistence.
// Totals survive reboot; CPS is a rolling 1000 ms window over press events.

#include <Arduino.h>
#include "config.h"

class Stats {
 public:
  void begin(uint8_t& activeLayer);
  void recordPress(uint8_t keyIndex);
  uint16_t cps();
  uint32_t total(uint8_t keyIndex) const;
  uint32_t grandTotal() const;
  void setVolume(uint8_t vol, bool muted);
  void save(uint8_t activeLayer);      // throttled write (periodic autosave)
  void saveNow(uint8_t activeLayer);   // unthrottled (layer changes)

 private:
  static constexpr uint8_t kCpsBuf = 64;
  uint32_t totals_[5] = {0, 0, 0, 0, 0};
  uint32_t pressTimes_[kCpsBuf] = {0};
  uint8_t pressHead_ = 0;
  uint8_t volume_ = 50;
  bool muted_ = false;
  uint32_t lastSaveMs_ = 0;
  void load(uint8_t& activeLayer);
  void write(uint8_t activeLayer);
};
