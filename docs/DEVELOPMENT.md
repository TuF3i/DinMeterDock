# DinMeterDock 二次开发指引

## 项目简介

DinMeterDock 是基于 M5DinMeter 硬件（ESP32-S3 + ST7789 240×135 屏幕）的桌面 Dock 控制器固件。项目使用 PlatformIO + Arduino 框架构建。

## 项目结构

```
DinMeterDock/
├── platformio.ini              # PlatformIO 构建配置
├── lib/                        # 本地 Arduino 库（不需要修改）
│   ├── Button/                 # 按键去抖
│   ├── ESP32Encoder/           # 旋转编码器 (PCNT)
│   └── I2C_BM8563/             # BM8563 RTC 驱动
├── src/
│   ├── main.cpp                # 入口：初始化硬件 → 启动菜单 → 主循环
│   ├── Config.h                # ★ 引脚定义 & 全局常量（改硬件接法只改这里）
│   ├── Hardware/               # 硬件抽象层
│   │   ├── Hardware.h          # 硬件门面，持有全部外设
│   │   ├── Display.h           # LGFX 屏幕 + 双缓冲画布
│   │   ├── Input.h             # 按键 + 旋转编码器
│   │   ├── RTC.h               # BM8563 实时时钟
│   │   ├── Buzzer.h            # 蜂鸣器
│   │   └── Power.h             # 电源保持/关机
│   ├── Apps/                   # ★ 应用层（二次开发主要在这里）
│   │   ├── AppBase.h           # 应用基类：virtual void run(Hardware& hw)
│   │   ├── DisplayApp.h        # 屏幕色彩测试
│   │   ├── BrightnessApp.h     # 背光亮度调节
│   │   ├── RtcTimeApp.h        # 实时时钟显示
│   │   ├── WifiScanApp.h       # WiFi 扫描
│   │   ├── EncoderApp.h        # 编码器读数
│   │   ├── ArkanoidApp.h       # 打砖块游戏
│   │   ├── SleepWakeupApp.h    # 定时休眠唤醒
│   │   └── PowerOffApp.h       # 关机
│   └── UI/                     # 用户界面层
│       ├── Launcher.h          # 启动器类声明
│       ├── Launcher.cpp        # 启动器实现（菜单动画 + App 注册表）
│       └── Assets/             # 图标 & 图片资源（头文件中的像素数组）
└── docs/
    └── DEVELOPMENT.md          # 本文档
```

## 架构概览

```
┌──────────────┐
│   main.cpp   │  setup() → hw.init() → launcher.begin()
│              │  loop()  → launcher.update()
└──────┬───────┘
       │
┌──────▼───────────────────────────────────────┐
│                  Launcher                     │
│  ┌──────────────────────────────────────┐    │
│  │ App 注册表 (名字/颜色/图标/回调)      │    │
│  │ [DisplayApp] [BrightnessApp] [RTC...] │    │
│  └──────────────────────────────────────┘    │
│  编码器切换选项 → 按键确认 → run(App)        │
└──────┬───────────────────────────────────────┘
       │  所有 App 通过 Hardware& 参数访问硬件
┌──────▼───────────────────────────────────────┐
│                Hardware                      │
│  Display  │  Input  │  RTC  │  Buzzer │ Power│
│  canvas   │  btn    │  time │  tone   │ hold │
│  push()   │  enc    │ alarm │  click  │ off  │
└──────────────────────────────────────────────┘
```

三层职责：

| 层 | 职责 | 修改频率 |
|----|------|---------|
| `Hardware/` | 封装硬件外设，提供简洁 API | 换了硬件接法才改 |
| `Apps/` | 每个应用独立实现 `run(Hardware&)` | 新增/修改功能时改 |
| `UI/` | 菜单动画、App 注册表、图标资源 | 新增 App 时加一行 |

## 新增一个 App

以新增一个"秒表"为例，三步完成：

### 1. 创建 App 文件

在 `src/Apps/` 新建 `StopwatchApp.h`：

```cpp
#pragma once
#include "AppBase.h"

class StopwatchApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        auto& c = hw.display.canvas;

        uint32_t start = millis();
        hw.input.resetEncoder(0);

        while (1)
        {
            // 绘制
            c->fillScreen(TFT_BLACK);
            c->setTextColor(TFT_WHITE);
            c->setTextSize(3);
            c->setCursor(30, 50);
            c->printf("STOPWATCH: %.1fs", (millis() - start) / 1000.0f);
            hw.display.push();

            // 编码器归零
            hw.input.checkEncoder(false);

            // 按键退出
            if (hw.input.checkNext()) break;
        }
    }
};
```

### 2. 准备图标（可选）

用 `lcd-image-converter` 工具将 32×32 PNG 转为 RGB565 数组，放到 `src/UI/Assets/stopwatch_icon.h`，然后在 `src/UI/Assets/assets.h` 中 `#include` 它。

如果没有图标，可以复用项目中其他图标的 `image_data_icon_*` 变量名。

### 3. 注册到菜单

编辑 `src/UI/Launcher.cpp`，在 `_apps` 数组末尾加一行：

```cpp
{"STOPWATCH", 0x87CEFA, 0x1E3A5F, image_data_icon_encoder, [](Hardware& h) { StopwatchApp().run(h); }},
// ^名称      ^主题色    ^文字色    ^图标                    ^运行回调
```

同时更新 `APP_COUNT` 常量（在 `Launcher.h` 中）为 `9`。

### 4. 编译

```bash
pio run
```

## 硬件 API 速查

所有 App 通过 `Hardware& hw` 访问硬件：

```cpp
// --- Display ---
hw.display.canvas         // LGFX_Sprite* 画布对象
hw.display.push()         // 将画布刷新到屏幕
hw.display.setBrightness(v)  // 0~255
hw.display.getBrightness()
hw.display.fillScreen(color)  // 直接填屏（绕过 canvas）

// --- Input ---
hw.input.btn_pwr.read()   // 读到 HIGH=释放 LOW=按下
hw.input.enc.getPosition()   // 读取编码器位置
hw.input.enc_pos           // 上次 checkEncoder 时的缓存值
hw.input.checkEncoder(playBuzz)  // 如位置变了返回 true，可选蜂鸣
hw.input.checkNext()       // 短按返回 true；长按触发关机不返回
hw.input.waitNext()        // 阻塞直到按键
hw.input.resetEncoder(pos) // 重置编码器到指定值

// --- RTC ---
hw.rtc.getTime(&timeStruct)  // I2C_BM8563_TimeTypeDef {hours,minutes,seconds}
hw.rtc.setAlarmIRQ(seconds)  // 设置闹钟唤醒
hw.rtc.clearIRQ()
hw.rtc.disableIRQ()

// --- Buzzer ---
hw.buzzer.tone(freq, duration_ms)
hw.buzzer.noTone()
hw.buzzer.click()          // 随机短促音效（游戏用）

// --- Power ---
hw.power.hold()            // 拉高电源保持引脚
hw.power.off()             // 关机（等待按键释放 → 拉低保持引脚）
hw.power.cut()             // 立即断电（用于休眠唤醒场景）
```

## 修改硬件引脚

所有引脚定义集中在 `src/Config.h`，改了接法只需修改这一个文件：

```cpp
// 编码器
#define ENC_PIN_A 40
#define ENC_PIN_B 41

// 按键
#define BTN_PWR_PIN     42
#define BTN_DEBOUNCE_MS 20

// 屏幕
#define LCD_MOSI_PIN 5
#define LCD_SCLK_PIN 6
#define LCD_DC_PIN   4
#define LCD_CS_PIN   7
#define LCD_RST_PIN  8
#define LCD_BL_PIN   9

// RTC
#define RTC_SDA_PIN 11
#define RTC_SCL_PIN 12

// 电池
#define BAT_ADC_PIN 10
#define BAT_DIVIDER 2.0f

// 蜂鸣器
#define BUZZ_PIN 3

// 电源
#define POWER_HOLD_PIN 46
```

## App 编写规范

每个 App 遵循以下模式：

```cpp
class MyApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        // 1. 准备阶段：设字体、重置编码器、初始化变量
        hw.display.canvas->setFont(&fonts::Font0);
        hw.input.resetEncoder(0);

        // 2. 主循环
        while (1)
        {
            // 2a. 绘制到 canvas
            hw.display.canvas->fillScreen(...);
            // ... 各种绘图操作 ...
            hw.display.push();   // 刷新到屏幕

            // 2b. 处理输入
            if (hw.input.checkEncoder()) { /* 编码器变化 */ }
            if (hw.input.checkNext())  break;  // 退出

            // 2c. 如需低 CPU 占用，加 delay
            // delay(10);  // optional
        }

        // 3. 清理（通常不需要，因为每次 run 创建新的 App 实例）
    }
};
```

**注意事项**：
- `run()` 中的 `while(1)` 循环在 App 运行时独占；退出循环即返回菜单
- App 实例在进入时创建、退出时销毁，成员变量会自然释放
- 不要在 `run()` 中 `delete` 任何 `hw` 中的对象，它们由 `Hardware` 统一管理
- 较长的 App（如 Arkanoid）应将游戏状态作为类成员，在构造函数或 `run()` 开头初始化

## 构建和烧录

```bash
# 编译
pio run

# 编译并烧录
pio run --target upload

# 串口监视
pio device monitor
```

默认环境 `esp32-s3-devkitc-1`，波特率 115200。

## 远程依赖

`platformio.ini` 中配置的 PlatformIO 注册表库：

| 库 | 版本 | 用途 |
|----|------|------|
| `lovyan03/LovyanGFX` | 1.1.12 | TFT 屏幕驱动和 2D 绘图 |
| `forairaaaaa/SmoothUIToolKit` | 1.0.1 | 菜单动画、缓动路径 |

本地 `lib/` 目录中的库随项目携带，不需要单独安装。
