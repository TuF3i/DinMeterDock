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
