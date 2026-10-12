#pragma once
// SSD1306 status UI: splash, layer header, 5 key cells with live press
// highlight, CPS/total footer, volume bar, layer-change toast.
// Refresh throttled to DISPLAY_FPS — never blocks the 1 ms USB scan.

#include <Arduino.h>
#include "config.h"

class Display {
 public:
  bool begin();
  void splash();
  void toast(const char* line1, const char* line2);
  void render(uint8_t activeLayer, const bool pressed[5],
              uint16_t cpsValue, uint32_t grandTotal,
              uint8_t volume, bool muted);

 private:
  bool ok_ = false;
  uint32_t lastFrameMs_ = 0;
  uint32_t toastUntilMs_ = 0;
  char toastA_[24] = {0};
  char toastB_[24] = {0};
  void drawVolume(uint8_t volume, bool muted);
};
