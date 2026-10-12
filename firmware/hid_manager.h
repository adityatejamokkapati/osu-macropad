#pragma once
// TinyUSB composite HID: Report ID 1 = NKRO keyboard, Report ID 2 = consumer.
// NKRO report payload (excluding ID): mods(1) + bitmap(15) covering usages 0x00..0x77.

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "config.h"

class HidManager {
 public:
  bool begin();
  // Returns true if the report changed and was sent.
  bool setKey(uint8_t usage, uint8_t mods, bool pressed);
  void releaseAllKeys();
  bool sendConsumer(uint16_t usage);  // press+release pulse
  bool ready();
  void task();

 private:
  bool sendNkro();
  uint8_t mods_ = 0;
  uint8_t bitmap_[15] = {0};
  // refcount per usage so two layers/keys sharing a usage don't ghost
  uint8_t refcount_[0x78] = {0};
  uint8_t modRef_[8] = {0};
  Adafruit_USBD_HID usbHid_;
  bool started_ = false;
};
