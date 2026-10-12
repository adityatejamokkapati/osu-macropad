#include "display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "keymap.h"

namespace {
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
}  // namespace

bool Display::begin() {
  Wire.begin();  // SDA=A4, SCL=A5 on Nano RP2040 Connect
  Wire.setClock(400000);
  ok_ = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
#if SERIAL_DEBUG
  if (!ok_) Serial.println(F("[oled] SSD1306 not found at 0x3C — check wiring/order"));
#endif
  if (ok_) {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
  }
  return ok_;
}

void Display::splash() {
  if (!ok_) return;
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(8, 8);
  oled.print(F("osu!pad"));
  oled.setTextSize(1);
  oled.setCursor(8, 34);
  oled.print(F("TinyUSB NKRO 1000Hz"));
  oled.setCursor(8, 46);
  oled.print(F("hold ENC = layer"));
  oled.display();
}

void Display::toast(const char* line1, const char* line2) {
  strncpy(toastA_, line1, sizeof(toastA_) - 1);
  strncpy(toastB_, line2, sizeof(toastB_) - 1);
  toastUntilMs_ = millis() + 1200;
}

void Display::drawVolume(uint8_t volume, bool muted) {
  oled.setCursor(0, 56);
  oled.print(muted ? F("MUTE") : F("VOL "));
  int16_t x = 34, w = 94, h = 6;
  oled.drawRect(x, 56, w, h, SSD1306_WHITE);
  if (!muted) {
    int16_t fill = (int16_t)((w - 2) * volume / 100);
    oled.fillRect(x + 1, 57, fill, h - 2, SSD1306_WHITE);
  } else {
    oled.setCursor(x + 30, 56);
    oled.print(F("--"));
  }
}

void Display::render(uint8_t activeLayer, const bool pressed[5],
                     uint16_t cpsValue, uint32_t grandTotal,
                     uint8_t volume, bool muted) {
  if (!ok_) return;
  uint32_t now = millis();
  if (now - lastFrameMs_ < (1000 / DISPLAY_FPS)) return;
  lastFrameMs_ = now;

  oled.clearDisplay();

  // Toast overlay (layer change) takes over the screen briefly.
  if (now < toastUntilMs_) {
    oled.setTextSize(2);
    oled.setCursor(10, 12);
    oled.print(toastA_);
    oled.setTextSize(1);
    oled.setCursor(10, 40);
    oled.print(toastB_);
    oled.display();
    return;
  }

  const LayerDef& L = LAYERS[activeLayer % NUM_LAYERS];

  // Header: layer name + position dots.
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(L.name);
  for (uint8_t i = 0; i < NUM_LAYERS; i++) {
    oled.setCursor((int16_t)(100 + i * 10), 0);
    oled.print(i == activeLayer ? F("(o)") : F(" o "));
  }

  // Key cells: 5 boxes across 128 px => 24 px each + gaps.
  for (uint8_t i = 0; i < 5; i++) {
    int16_t x = (int16_t)(i * 25 + 1);
    int16_t y = 12;
    if (pressed[i]) {
      oled.fillRect(x, y, 23, 22, SSD1306_WHITE);
      oled.setTextColor(SSD1306_BLACK);
    } else {
      oled.drawRect(x, y, 23, 22, SSD1306_WHITE);
      oled.setTextColor(SSD1306_WHITE);
    }
    oled.setCursor(x + 2, y + 3);
    // Labels are short (<=4 chars); clip defensively.
    char buf[5] = {0};
    strncpy(buf, L.keys[i].label, 4);
    oled.print(buf);
    oled.setCursor(x + 2, y + 13);
    oled.print((int)(i + 1));
    oled.setTextColor(SSD1306_WHITE);
  }

  // Footer: CPS + grand total.
  oled.setCursor(0, 44);
  oled.print(F("CPS "));
  oled.print(cpsValue);
  oled.setCursor(64, 44);
  oled.print(F("TOT "));
  oled.print((unsigned long)grandTotal);

  drawVolume(volume, muted);
  oled.display();
}
