# osu-macropad firmware

Gaming-grade USB macropad firmware for the **Arduino Nano RP2040 Connect**
(Earle Philhower `arduino-pico` core + TinyUSB).

* 5x Cherry MX keys, active-LOW, 1 ms scan, 5 ms debounce, NKRO HID
* EC11 encoder (volume / layer control) + push-button (short = mute, long = layer)
* 0.96" SSD1306 OLED (I2C) with layer menu, key state, CPS, volume bar
* 3 layers: `osu!` / `MEDIA` / `WORK`, persisted to LittleFS

## Verified pin map (from `osu macro pad.kicad_pcb`)

| Key | PCB ref | Nano pin | Arduino name |
|---|---|---|---|
| K1 (btn1) | pad 2 | D0/RX | `D0` |
| K2 (btn2) | pad 2 | D6 | `D6` |
| K3 (btn3) | pad 2 | D7 | `D7` |
| K4 (btn4) | pad 2 | D8 | `D8` |
| K5 (btn5) | pad 2 | D5 | `D5` |
| Enc A | SW1-A | D2 | `D2` |
| Enc B | SW1-B | D3 | `D3` |
| Enc btn | SW1-S1 | D4 | `D4` |
| OLED SDA | pin 4 | SDA/A4 | `A4` |
| OLED SCL | pin 3 | SCL/A5 | `A5` |
| OLED VCC/GND | pins 2/1 | 3V3 / GND | — |

All switch commons go to GND. Firmware uses `INPUT_PULLUP`.

## OLED header (verified from `oledscreen1` footprint in the PCB)

4-pin I2C, 2.54 mm pitch, pin 1 = square pad:

| Header pin | Signal | Nano pin |
|---|---|---|
| 1 | GND | GND |
| 2 | 3V3 | 3V3 |
| 3 | SCL (clock — also labeled SCK on some modules) | A5 |
| 4 | SDA (data) | A4 |

Firmware drives this as SSD1306 128x64 @ I2C address `0x3C`
(`Wire.begin()` + `oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)` —
already matching, no config needed).

Compatible with any 0.96" SSD1306 I2C module with pin order
`GND / VCC / SCL / SDA` (the most common variant).
> Not plug-compatible: modules ordered `VCC / GND / SCL / SDA`
> (power pins swapped — would short 3V3 to GND). Check your
> module's silkscreen before inserting.

## Requirements

* Arduino IDE 2.x **or** `arduino-cli`
* Board core: `rp2040:rp2040` by Earle Philhower (>= 3.9.0)
  * Arduino IDE: Boards Manager → search `rp2040` → install
    "Raspberry Pi Pico/RP2040" by Earle Philhower.
  * `arduino-cli core install rp2040:rp2040`
* Libraries (Library Manager):
  * `Adafruit GFX Library`
  * `Adafruit SSD1306`
  * (TinyUSB is bundled with the pico core — do not install separately)
* Board settings:
  * Board: **Arduino Nano RP2040 Connect** (under RP2040 boards)
  * USB Stack: **Adafruit TinyUSB**
  * CPU Speed: **133 MHz**
  * Flash Size: **16MB (FS: 1MB)**
  * Debug Port / Level: default

## Build & flash

Arduino IDE: open `firmware/firmware.ino`, select the board + port,
click Upload. If the board is stuck, double-press RESET to enter
UF2 bootloader, then upload again.

CLI:

```sh
arduino-cli core install rp2040:rp2040
arduino-cli lib install "Adafruit GFX Library" "Adafruit SSD1306"
arduino-cli compile -b rp2040:rp2040:arduino_nano_connect:usbstack=tinyusb firmware
arduino-cli upload -b rp2040:rp2040:arduino_nano_connect:usbstack=tinyusb -p /dev/ttyACM0 firmware
```

Adjust `/dev/ttyACM0` to your port (COMx on Windows).

## Editing the keymap

Open `keymap.h`. Each layer is 5 `Action`s:

```cpp
// KEY action:   ACTION_KEY(HID_USAGE, MODS, "label")
// CONSUMER:     ACTION_CONS(0xCD, "label")
```

HID usages (subset): `Z=0x1D X=0x1B C=0x06 V=0x19 S=0x16 ESC=0x29
SPACE=0x2C F5=0x3E UP=0x52 DOWN=0x51 LEFT=0x50 RIGHT=0x4F ENTER=0x28`.
Modifiers: `MOD_NONE=0 MOD_CTRL=1 MOD_SHIFT=2 MOD_ALT=4 MOD_GUI=8`
(combine with `|`, e.g. `MOD_CTRL` for Ctrl+C).

Consumer usages: `VOL_UP=0xE9 VOL_DOWN=0xEA MUTE=0xE2
PLAY=0xCD NEXT=0xB5 PREV=0xB6`.

Encoder per-layer rotate + shared button behaviour live in the same file.

## Tuning (`config.h`)

* `DEBOUNCE_MS` (default 5) — raise to 10–20 if a switch chatters.
* `SCAN_INTERVAL_US` (default 1000 = 1 ms) — do not raise for osu!.
* `ENC_PULSES_PER_DETENT` (default 2 for EC11) — set to 1/2/4 if volume
  steps feel doubled/halved.
* `LONG_PRESS_MS` (default 600), `DISPLAY_FPS`, `OLED_ADDR 0x3C`.
* `SOCD_LAST_WINS` — leave 0 unless Z+X simultaneous handling is wanted.

## Serial debug

Open 115200 baud monitor. The firmware prints layer changes, key events
(if `SERIAL_DEBUG 1`), CPS every 2 s, and I2C scan failures.
HID works even with the monitor closed.

## Bring-up checklist

1. Flash, check OLED splash `osu!pad ready`.
2. Press K1/K2 → `z`/`x` typed in a text editor.
3. Rotate encoder → OS volume changes; short-press → mute.
4. Long-press encoder (>600 ms) → layer cycles osu! → MEDIA → WORK,
   OLED toast confirms, layer persists across reboot.
5. Soak-test 10 min in osu! — if double-hits, raise `DEBOUNCE_MS`.
6. If encoder direction feels inverted, set `ENCODER_REVERSE 1`.
