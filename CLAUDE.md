# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DinMeterDock is a desktop Dock controller firmware for the M5DinMeter hardware (ESP32-S3 + ST7789 240×135 TFT display). Built with PlatformIO using the Arduino framework.

## Build & Development Commands

```bash
# Compile
pio run

# Compile and upload to device
pio run --target upload

# Serial monitor (115200 baud)
pio device monitor

# Run PlatformIO unit tests
pio test

# Run a single test
pio test --filter <test_name>
```

One build environment: `esp32-s3-devkitc-1` (defined in `platformio.ini`). CPU frequency locked at 240MHz.

## Architecture

Three-layer design, all coordinated through `main.cpp`:

```
main.cpp (setup → loop)
  └─ Hardware (owns all peripherals, passed by reference to everything)
  └─ Launcher (menu system with animated app selector)
       └─ Apps (individual full-screen applications)
```

| Layer | Location | Role | When to modify |
|-------|----------|------|---------------|
| **Hardware** | `src/Hardware/` | Owns and wraps peripherals (display, input, RTC, buzzer, power) | Only when pin assignments change |
| **Apps** | `src/Apps/` | Each is a self-contained screen/feature implementing `AppBase::run(Hardware&)` | Adding features or modifying behavior |
| **UI** | `src/UI/` | Menu launcher, app registry, icon assets | Adding a new app entry |

### Control flow

Apps run cooperatively (not preemptively). `AppBase::run()` contains a `while(1)` loop that draws to the canvas, pushes to display, polls input, and breaks to return to the menu. While an app runs, `launcher.update()` is blocked — the app owns the main loop.

### Hardware facade (`src/Hardware/Hardware.h`)

`Hardware` is a plain struct passed by reference to every App. Members:
- **`display`** — `LGFX_Sprite* canvas` (double-buffered; draw on this, then `push()`). Uses LovyanGFX with ST7789 panel over SPI at 40MHz.
- **`input`** — Button (`btn_pwr`) + ESP32Encoder (`enc`). Button uses hardware pull-up: LOW = pressed, HIGH = released. Encoder uses PCNT half-quad mode.
- **`rtc`** — BM8563 over I2C (100kHz). Supports alarm IRQ for sleep/wake.
- **`buzzer`** — Single GPIO tone output.
- **`power`** — GPIO46 hold pin. Must be held HIGH to keep device powered.

### App pattern

Every app follows the same structure (see `src/Apps/AppBase.h`):
1. Get `hw.display.canvas` (an `LGFX_Sprite*`)
2. Call `hw.input.resetEncoder(pos)` to initialize encoder state
3. Enter `while(1)` loop: draw → `hw.display.push()` → poll `hw.input.checkEncoder()` / `hw.input.checkNext()` → (optional) `delay()`
4. `break` from loop to return to menu

Apps are instantiated, run, and destroyed — member variables are cleaned up naturally.

### Pin definitions

All pins are centralized in `src/Config.h`. Key assignments: encoder A=40 B=41, power button=42, SPI display MOSI=5 SCLK=6 DC=4 CS=7 RST=8 BL=9, RTC I2C SDA=11 SCL=12, battery ADC=10 (with 2:1 divider), buzzer=3, power hold=46.

### Local libraries (`lib/`)

Three project-specific libraries (compiled as static libs by PlatformIO):
- **Button** — debounced GPIO button
- **ESP32Encoder** — PCNT-based rotary encoder
- **I2C_BM8563** — BM8563 RTC driver over I2C

These should not be modified.

### Remote dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| `lovyan03/LovyanGFX` | 1.1.12 | TFT display driver and 2D graphics |
| `forairaaaaa/SmoothUIToolKit` | 1.0.1 | Menu animations, easing paths, transitions |
| `knolleary/PubSubClient` | ^2.8 | MQTT client (for planned network feature) |

## Planned Feature: MQTT / Network

The `docs/MQTT.md` document describes a planned network layer (`src/Network/`) with:
- `MessageBus` — ring-buffer queue decoupling MQTT receive from UI consumption
- `NetworkService` — WiFi STA + MQTT client with auto-reconnect
- Integration into `Hardware` as `hw.network`
- App pattern: `while (hw.network.pollMessage(msg)) { handleMessage(msg); }` inside the main loop

This has **not yet been implemented** (no `src/Network/` directory exists).

## Testing

PlatformIO Unit Testing framework is configured (`test/` directory). Tests are run on the target hardware (ESP32-S3), not on the host. No tests have been written yet.

## VSCode / Editor

Recommended extension: `platformio.platformio-ide`. Pre-configured debug launch tasks in `.vscode/launch.json` (PIO Debug, skip Pre-Debug, without uploading).

## Adding a New App

1. Create `src/Apps/MyApp.h` extending `AppBase`, implement `run(Hardware& hw)` following the App pattern
2. Optionally create a 32×32 RGB565 icon and include it in `src/UI/Assets/assets.h`
3. Add an entry to the `_apps` array in `src/UI/Launcher.cpp` (name, theme/tag colors, icon, lambda)
4. Bump `APP_COUNT` in `src/UI/Launcher.h`
5. Run `pio run` to build

## Icons

### Format

All icons are 32×32 pixels, RGB565 format (`uint16_t[1024]`), stored as C header arrays in `src/UI/Assets/`. They are rendered in the Launcher via `canvas->pushImage(x, y, 32, 32, data)`.

### Transparency

**`pushImage()` has no alpha channel** — every pixel overwrites whatever is underneath. To make icon backgrounds transparent so the menu card's `themeColor` shows through:

1. Set the icon's background pixels to `0x0000` (pure black in RGB565)
2. Use the transparent overload: `canvas->pushImage(x, y, 32, 32, data, (uint16_t)0x0000)`
3. LovyanGFX will skip `0x0000` pixels, leaving the underlying card color visible in those spots

This is already applied globally in `Launcher.cpp` line 123. All icons that use `0x0000` as background will automatically have transparency. **Current icons do not use `0x0000` in their graphics, so this is safe.** If you create a new icon that needs `0x0000` in the foreground graphic, use a different sentinel value (e.g., `0x0001`) and update the Launcher pushImage call accordingly.

### Generation scripts

```bash
# Convert PNG to icon header (requires Pillow)
python3 scripts/png2icon.py icon.png --name icon_xxx --bg-color RRGGBB -o ../src/UI/Assets/

# Generate a gear icon programmatically (no deps)
python3 scripts/gear_icon.py > src/UI/Assets/icon_menu.h
```

`png2icon.py` supports:
- `--bg-color RRGGBB` — replace transparent (alpha < 128) pixels with 24-bit color
- `--bg-color-565 HEX` — replace with exact RGB565 value (skips conversion)
- `--width W --height H` — non-standard sizes (default 32×32)

## Buzzer / Volume

The buzzer (GPIO3) uses ESP32 LEDC PWM channel 6 (channel 7 is used by display backlight). `Buzzer.h` wraps `ledcSetup`/`ledcAttachPin`/`ledcWrite`/`ledcDetachPin` directly — it does **not** use Arduino's `tone()`.

- Volume range: 0-255 (default 128 = 50% duty)
- `Input.h` routes through `Buzzer` when `_buzzer` is set (done in `Hardware::init()`)
- Volume is adjustable via `BuzzerVolumeApp` in the Settings menu

## Settings Sub-Menu

`SettingsApp.h` is a vertical row-list sub-menu using `SmoothUIToolKit::SelectMenu::SmoothSelector` (not `SmoothOptions` like the main Launcher).

Key patterns:
- **Row layout**: each option has a fixed keyframe. Selected row keyframe.x is set to 12 (slides right), others at 0. The `SmoothSelector` animates the highlight between positions.
- **Scrolling**: `setCameraSize(240, 110)` enables the camera. Rows are offset by `getCameraOffset().y` in `onRender()`. Title bar is drawn at fixed y=0.
- **Back option**: the last entry has `runner = nullptr`. `onClick()` checks this and sets `goBack = true` instead of opening.
- **Row clipping**: skip rows with `rowY < 25` (title bar area) or `rowY + rowH <= 25` or `rowY >= 135`.
- **Encoder init**: `lastEncPos` MUST sync to `hw.input.enc.getPosition()` after `resetEncoder()` to prevent cursor jump.
- Adding an item: bump `SETTING_COUNT`, add to `_settings[]` array and `rowY[]` array in `run()`, add corresponding `#include`.
