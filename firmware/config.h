#pragma once
// Central tunables for osu-macropad.
// Pin map verified against osu macro pad.kicad_pcb nets.

#include <Arduino.h>

// ---- Key pins (active-LOW, commons to GND, INPUT_PULLUP) ----
// Order = KEY0..KEY4 maps to PCB btn1..btn5.
static constexpr uint8_t KEY_PINS[5] = { D0, D6, D7, D8, D5 };
static constexpr uint8_t NUM_KEYS = 5;

// ---- Encoder (EC11, commons to GND) ----
static constexpr uint8_t ENC_PIN_A   = D2;  // SW1-A, Net-(A1-D2)
static constexpr uint8_t ENC_PIN_B   = D3;  // SW1-B, Net-(A1-D3)
static constexpr uint8_t ENC_PIN_BTN = D4;  // SW1-S1, Net-(A1-D4), active-LOW

// Set to 1 if clockwise decreases volume on your EC11 batch.
#ifndef ENCODER_REVERSE
#define ENCODER_REVERSE 0
#endif
// EC11 usually yields 2 state transitions per detent with a
// full-step decoder; accumulate this many before emitting one step.
#ifndef ENC_PULSES_PER_DETENT
#define ENC_PULSES_PER_DETENT 2
#endif

// ---- OLED (SSD1306 128x64, I2C) ----
// PCB oledscreen1: 1=GND 2=3V3 3=SCL/A5 4=SDA/A4
static constexpr uint8_t OLED_ADDR   = 0x3C;
static constexpr uint8_t OLED_WIDTH  = 128;
static constexpr uint8_t OLED_HEIGHT = 64;

// ---- Timing ----
static constexpr uint16_t SCAN_INTERVAL_US = 1000;  // 1 ms = 1000 Hz scan
static constexpr uint8_t  DEBOUNCE_MS      = 5;     // per-key, osu!-safe
static constexpr uint16_t LONG_PRESS_MS    = 600;   // encoder btn: short vs layer-cycle
static constexpr uint8_t  DISPLAY_FPS      = 15;    // OLED refresh (USB loop stays 1 ms)
static constexpr uint16_t CPS_WINDOW_MS    = 1000;  // clicks-per-second window

// ---- Layers ----
static constexpr uint8_t NUM_LAYERS = 3;

// ---- HID ----
static constexpr uint8_t HID_POLL_INTERVAL_MS = 1;  // 1000 Hz USB poll request

// ---- Optional behaviours ----
#ifndef SOCD_LAST_WINS
#define SOCD_LAST_WINS 0  // 1 = opposite keys (Z+X) resolve last-wins; 0 = both reported
#endif

#ifndef SERIAL_DEBUG
#define SERIAL_DEBUG 1  // 1 = layer/key/CPS logs at 115200 baud
#endif

// ---- HID modifier bits ----
static constexpr uint8_t MOD_NONE  = 0x00;
static constexpr uint8_t MOD_CTRL  = 0x01;
static constexpr uint8_t MOD_SHIFT = 0x02;
static constexpr uint8_t MOD_ALT   = 0x04;
static constexpr uint8_t MOD_GUI   = 0x08;

// ---- HID keyboard usages (USB HID Usage Tables, page 0x07) ----
static constexpr uint8_t HID_A = 0x04, HID_B = 0x05, HID_C = 0x06, HID_D = 0x07;
static constexpr uint8_t HID_S = 0x16, HID_V = 0x19, HID_X = 0x1B, HID_Z = 0x1D;
static constexpr uint8_t HID_ENTER = 0x28, HID_ESC = 0x29, HID_SPACE = 0x2C;
static constexpr uint8_t HID_F5 = 0x3E;
static constexpr uint8_t HID_LEFT = 0x50, HID_RIGHT = 0x4F;
static constexpr uint8_t HID_UP = 0x52, HID_DOWN = 0x51;

// ---- HID consumer usages (page 0x0C, 16-bit) ----
static constexpr uint16_t CONS_MUTE     = 0x00E2;
static constexpr uint16_t CONS_VOL_UP   = 0x00E9;
static constexpr uint16_t CONS_VOL_DOWN = 0x00EA;
static constexpr uint16_t CONS_PLAY     = 0x00CD;
static constexpr uint16_t CONS_NEXT     = 0x00B5;
static constexpr uint16_t CONS_PREV     = 0x00B6;
