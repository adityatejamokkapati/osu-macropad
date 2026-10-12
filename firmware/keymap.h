#pragma once
// Default keymap: 3 layers x 5 keys + per-layer encoder behaviour.
// Edit labels/actions here — no other file needs to change.

#include "config.h"

enum class ActionType : uint8_t { None = 0, Key = 1, Consumer = 2 };

struct Action {
  ActionType type = ActionType::None;
  uint8_t key = 0;        // HID keyboard usage (if Key)
  uint8_t mods = MOD_NONE;  // modifier bitmask (if Key)
  uint16_t consumer = 0;  // HID consumer usage (if Consumer)
  const char* label = "";
};

#define ACTION_NONE() Action{ActionType::None, 0, MOD_NONE, 0, ""}
#define ACTION_KEY(usage, modifiers, text) Action{ActionType::Key, usage, modifiers, 0, text}
#define ACTION_CONS(usage, text) Action{ActionType::Consumer, 0, MOD_NONE, usage, text}

struct LayerDef {
  const char* name;
  const char* shortName;
  Action keys[5];          // KEY0..KEY4 = PCB btn1..btn5
  Action encCW;            // encoder clockwise
  Action encCCW;           // encoder counter-clockwise
  Action encShortPress;    // encoder button tap
};

// Long-press (hold > LONG_PRESS_MS) always cycles layer; defined in .ino.

static constexpr LayerDef LAYERS[NUM_LAYERS] = {
  { // Layer 0 — osu!
    "osu!",
    "OSU",
    {
      ACTION_KEY(HID_Z, MOD_NONE, "Z"),
      ACTION_KEY(HID_X, MOD_NONE, "X"),
      ACTION_KEY(HID_ESC, MOD_NONE, "ESC"),
      ACTION_KEY(HID_F5, MOD_NONE, "F5"),
      ACTION_KEY(HID_SPACE, MOD_NONE, "SPC"),
    },
    ACTION_CONS(CONS_VOL_UP, "VOL+"),
    ACTION_CONS(CONS_VOL_DOWN, "VOL-"),
    ACTION_CONS(CONS_MUTE, "MUTE"),
  },
  { // Layer 1 — media
    "MEDIA",
    "MED",
    {
      ACTION_CONS(CONS_PREV, "PREV"),
      ACTION_CONS(CONS_PLAY, "PLAY"),
      ACTION_CONS(CONS_NEXT, "NEXT"),
      ACTION_CONS(CONS_MUTE, "MUTE"),
      ACTION_KEY(HID_SPACE, MOD_NONE, "SPC"),
    },
    ACTION_CONS(CONS_VOL_UP, "VOL+"),
    ACTION_CONS(CONS_VOL_DOWN, "VOL-"),
    ACTION_CONS(CONS_MUTE, "MUTE"),
  },
  { // Layer 2 — productivity
    "WORK",
    "WRK",
    {
      ACTION_KEY(HID_C, MOD_CTRL, "COPY"),
      ACTION_KEY(HID_V, MOD_CTRL, "PSTE"),
      ACTION_KEY(HID_X, MOD_CTRL, "CUT"),
      ACTION_KEY(HID_Z, MOD_CTRL, "UNDO"),
      ACTION_KEY(HID_S, MOD_CTRL, "SAVE"),
    },
    ACTION_KEY(HID_DOWN, MOD_NONE, "DOWN"),
    ACTION_KEY(HID_UP, MOD_NONE, "UP"),
    ACTION_CONS(CONS_MUTE, "MUTE"),
  },
};
