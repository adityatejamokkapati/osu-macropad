#pragma once
// EC11 quadrature decoder (interrupt-driven) + push-button state machine.
// poll() is non-blocking: rotation accumulates detents, button reports
// short-press vs long-press events.

#include <Arduino.h>
#include "config.h"

class Encoder {
 public:
  void begin();
  void poll();  // call every 1 ms loop

  // Returns +1/-1 detents since last call (0 = none).
  int8_t consumeSteps();
  bool consumeShortPress();
  bool consumeLongPress();

 private:
  static void isrA();
  static void isrB();
  static void update(uint8_t a, uint8_t b);

  static volatile int16_t rawDelta_;
  static volatile uint8_t lastState_;

  int16_t accumulator_ = 0;
  bool shortFlag_ = false;
  bool longFlag_ = false;

  // Button debounce/state
  bool lastLevel_ = HIGH;
  uint32_t lastChangeMs_ = 0;
  uint32_t pressStartMs_ = 0;
  bool pressed_ = false;
  bool longFired_ = false;
};
