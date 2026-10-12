#include "encoder.h"

volatile int16_t Encoder::rawDelta_ = 0;
volatile uint8_t Encoder::lastState_ = 0;

// Full-step transition table: index = prev(2b):curr(2b) -> -1/0/+1.
static const int8_t kQuadTable[16] = {
   0, -1, +1,  0,
  +1,  0,  0, -1,
  -1,  0,  0, +1,
   0, +1, -1,  0,
};

void Encoder::begin() {
  pinMode(ENC_PIN_A, INPUT_PULLUP);
  pinMode(ENC_PIN_B, INPUT_PULLUP);
  pinMode(ENC_PIN_BTN, INPUT_PULLUP);
  lastState_ = (uint8_t)((digitalRead(ENC_PIN_A) << 1) | digitalRead(ENC_PIN_B));
  rawDelta_ = 0;
  attachInterrupt(digitalPinToInterrupt(ENC_PIN_A), isrA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_PIN_B), isrB, CHANGE);
  lastLevel_ = digitalRead(ENC_PIN_BTN);
  lastChangeMs_ = millis();
}

void Encoder::isrA() {
  update((uint8_t)digitalRead(ENC_PIN_A), (uint8_t)digitalRead(ENC_PIN_B));
}

void Encoder::isrB() {
  update((uint8_t)digitalRead(ENC_PIN_A), (uint8_t)digitalRead(ENC_PIN_B));
}

void Encoder::update(uint8_t a, uint8_t b) {
  uint8_t curr = (uint8_t)((a << 1) | b);
  uint8_t idx = (uint8_t)((lastState_ << 2) | curr);
  rawDelta_ += kQuadTable[idx & 0x0F];
  lastState_ = curr;
}

void Encoder::poll() {
  // Drain ISR accumulator into detent counter.
  noInterrupts();
  int16_t raw = rawDelta_;
  rawDelta_ = 0;
  interrupts();
#if ENCODER_REVERSE
  raw = -raw;
#endif
  accumulator_ += raw;

  // Button state machine with debounce.
  bool level = digitalRead(ENC_PIN_BTN);
  uint32_t now = millis();
  if (level != lastLevel_) {
    lastLevel_ = level;
    lastChangeMs_ = now;
    return;
  }
  if ((now - lastChangeMs_) < DEBOUNCE_MS) return;

  if (level == LOW && !pressed_) {
    pressed_ = true;
    pressStartMs_ = now;
    longFired_ = false;
  } else if (level == HIGH && pressed_) {
    pressed_ = false;
    if (!longFired_ && (now - pressStartMs_) < LONG_PRESS_MS) shortFlag_ = true;
  } else if (pressed_ && !longFired_ && (now - pressStartMs_) >= LONG_PRESS_MS) {
    longFired_ = true;
    longFlag_ = true;
  }
}

int8_t Encoder::consumeSteps() {
  int16_t steps = accumulator_ / ENC_PULSES_PER_DETENT;
  if (steps == 0) return 0;
  // Keep remainder so slow rotation never loses pulses.
  accumulator_ -= steps * (int16_t)ENC_PULSES_PER_DETENT;
  if (steps > 127) steps = 127;
  if (steps < -127) steps = -127;
  return (int8_t)steps;
}

bool Encoder::consumeShortPress() {
  bool v = shortFlag_;
  shortFlag_ = false;
  return v;
}

bool Encoder::consumeLongPress() {
  bool v = longFlag_;
  longFlag_ = false;
  return v;
}
