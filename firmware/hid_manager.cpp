#include "hid_manager.h"

// Report descriptor: ID1 NKRO keyboard + ID2 consumer control.
static uint8_t const hidReportDesc[] = {
  // --- NKRO keyboard, Report ID 1 ---
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x06,        // Usage (Keyboard)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x05, 0x07,        //   Usage Page (Key Codes)
  0x19, 0xE0, 0x29, 0xE7,  // Usage Min (Left Ctrl) - Max (Right GUI)
  0x15, 0x00, 0x25, 0x01,  // Logical Min/Max
  0x75, 0x01, 0x95, 0x08,  // Report Size 1, Count 8 (modifiers)
  0x81, 0x02,        //   Input (Data,Var,Abs)
  0x05, 0x08,        //   Usage Page (LEDs)
  0x19, 0x01, 0x29, 0x05,  // Usage Min/Max
  0x95, 0x05, 0x75, 0x01,  // Count 5, Size 1
  0x91, 0x02,        //   Output (Data,Var,Abs)
  0x95, 0x01, 0x75, 0x03,  // padding
  0x91, 0x03,        //   Output (Const,Var,Abs)
  0x05, 0x07,        //   Usage Page (Key Codes)
  0x19, 0x00, 0x29, 0x77,  // Usage Min 0x00 - Max 0x77 (120 keys)
  0x15, 0x00, 0x25, 0x01,
  0x75, 0x01, 0x95, 0x78,  // 120 x 1-bit = 15 bytes
  0x81, 0x02,        //   Input (Data,Var,Abs)
  0xC0,              // End Collection
  // --- Consumer control, Report ID 2 ---
  0x05, 0x0C,        // Usage Page (Consumer)
  0x09, 0x01,        // Usage (Consumer Control)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x02,        //   Report ID (2)
  0x19, 0x00, 0x2A, 0x3C, 0x02,  // Usage Min 0 - Max 0x023C
  0x15, 0x00, 0x26, 0x3C, 0x02,  // Logical Min/Max
  0x75, 0x10, 0x95, 0x01,  // 16-bit, 1 field
  0x81, 0x00,        //   Input (Data,Array,Abs)
  0xC0,              // End Collection
};

bool HidManager::begin() {
  usbHid_.setPollInterval(HID_POLL_INTERVAL_MS);
  usbHid_.setReportDescriptor(hidReportDesc, sizeof(hidReportDesc));
  usbHid_.setStringDescriptor("osu-macropad HID");
  started_ = usbHid_.begin();
  return started_;
}

bool HidManager::ready() {
  return started_ && TinyUSBDevice.mounted() && usbHid_.ready();
}

void HidManager::task() {
  // arduino-pico runs TinyUSB in the background; yield keeps it fed
  // when the sketch never blocks.
  yield();
}

bool HidManager::setKey(uint8_t usage, uint8_t mods, bool pressed) {
  if (usage >= 0x78 && !(usage >= 0xE0 && usage <= 0xE7)) return false;

  bool changed = false;

  // Modifier bits from the action (Ctrl/Shift/Alt/GUI).
  for (uint8_t b = 0; b < 8; b++) {
    uint8_t mask = (uint8_t)(1u << b);
    if (!(mods & mask)) continue;
    if (pressed) {
      if (modRef_[b] == 0) { mods_ |= mask; changed = true; }
      if (modRef_[b] < 255) modRef_[b]++;
    } else {
      if (modRef_[b] > 0) {
        modRef_[b]--;
        if (modRef_[b] == 0) { mods_ &= (uint8_t)~mask; changed = true; }
      }
    }
  }

  // Regular usage bitmap (modifiers 0xE0..0xE7 never appear as usages here).
  if (usage < 0x78) {
    uint8_t idx = (uint8_t)(usage >> 3);
    uint8_t bit = (uint8_t)(usage & 0x07);
    if (pressed) {
      if (refcount_[usage] == 0) { bitmap_[idx] |= (uint8_t)(1u << bit); changed = true; }
      if (refcount_[usage] < 255) refcount_[usage]++;
    } else {
      if (refcount_[usage] > 0) {
        refcount_[usage]--;
        if (refcount_[usage] == 0) { bitmap_[idx] &= (uint8_t)~(1u << bit); changed = true; }
      }
    }
  }

  if (changed) return sendNkro();
  return false;
}

void HidManager::releaseAllKeys() {
  mods_ = 0;
  memset(bitmap_, 0, sizeof(bitmap_));
  memset(refcount_, 0, sizeof(refcount_));
  memset(modRef_, 0, sizeof(modRef_));
  sendNkro();
}

bool HidManager::sendNkro() {
  if (!ready()) return false;
  uint8_t report[16];
  report[0] = mods_;
  memcpy(&report[1], bitmap_, 15);
  return usbHid_.sendReport(1, report, sizeof(report));
}

bool HidManager::sendConsumer(uint16_t usage) {
  if (!ready()) return false;
  // Press + release as two back-to-back reports (no delay: keeps the
  // 1 ms key scan non-blocking even during fast encoder spins;
  // TinyUSB queues both frames and hosts register the pulse).
  uint8_t press[2] = {(uint8_t)(usage & 0xFF), (uint8_t)(usage >> 8)};
  uint8_t release[2] = {0, 0};
  bool ok = usbHid_.sendReport(2, press, 2);
  ok &= usbHid_.sendReport(2, release, 2);
  return ok;
}
