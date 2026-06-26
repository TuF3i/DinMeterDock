# M5DinMeter-UserDemo 项目介绍

## 这是什么

`M5DinMeter-UserDemo` 是一个面向 M5 DinMeter 硬件评估的用户演示固件。项目基于 ESP32-S3 和 Arduino 框架开发，使用 PlatformIO 管理构建与依赖。

固件启动后会初始化电源保持、RTC、按键、屏幕和旋转编码器，然后进入一个图形化菜单。用户可以通过旋转编码器切换功能项，通过电源按键进入功能页。当前菜单包含：

- Display Test：屏幕显示与颜色测试。
- Brightness：背光亮度调节。
- RTC Time：读取并显示 BM8563 RTC 时间。
- WiFi Scan：扫描附近 Wi-Fi 网络并显示 RSSI 和加密方式。
- Encoder Test：显示旋转编码器计数。
- Arkanoid：一个用编码器控制挡板的打砖块小游戏。
- Sleep & Wake Up：设置 RTC 定时唤醒并进入关机/休眠流程。
- Power Off：关闭设备电源保持。

项目中还保留了工厂测试相关代码，可用于显示、RTC、编码器、IO、Wi-Fi、BLE 和 RTC 唤醒等硬件功能验证。

## 项目结构

```text
.
├── platformio.ini
├── README.md
├── src
│   ├── main.cpp
│   ├── view
│   │   ├── view.cpp
│   │   └── assets
│   └── factory_test
│       ├── factory_test.h
│       ├── factory_test.cpp
│       └── components
├── lib
│   ├── Button
│   ├── ESP32Encoder
│   └── I2C_BM8563
├── include
└── test
```

关键目录说明：

- `src/main.cpp`：Arduino 入口，负责初始化 `FactoryTest` 并启动 UI。
- `src/view/view.cpp`：用户界面菜单与动画逻辑。
- `src/view/assets`：菜单图标、面板图片等资源，头文件中存储图像数组。
- `src/factory_test/factory_test.h`：`FactoryTest` 类声明，集中定义各硬件模块接口。
- `src/factory_test/factory_test.cpp`：设备初始化和工厂测试流程入口。
- `src/factory_test/components`：各硬件/功能模块实现。
- `lib`：随项目携带的本地 Arduino 库，包括按键去抖、BM8563 RTC 和 ESP32 编码器。

## 怎么实现的

### 运行流程

项目使用 Arduino 的 `setup()` 和 `loop()` 模型：

1. `src/main.cpp` 中创建全局 `FactoryTest ft`。
2. `setup()` 调用 `ft.init()` 初始化硬件。
3. `setup()` 调用 `view_create(&ft)` 创建菜单界面。
4. `loop()` 持续调用 `view_update()`，由 UI 框架定时读取输入、更新动画并渲染屏幕。

`FactoryTest::init()` 的初始化顺序是：

1. `_power_on()`：拉高 `POWER_HOLD_PIN`，保持设备上电。
2. `_rtc_init()`：通过 I2C 初始化 BM8563 RTC。
3. `_key_init()`：初始化电源按键。
4. `_disp_init()`：初始化 LovyanGFX 屏幕对象和离屏画布。
5. 初始化旋转编码器：`_enc.attachHalfQuad(40, 41)`，并把计数清零。

### 图形界面

菜单在 `src/view/view.cpp` 中实现，核心类是继承自 `SmoothUIToolKit::SelectMenu::SmoothOptions` 的 `LauncherMenu`。

`LauncherMenu` 主要重写了这些回调：

- `onReadInput()`：读取旋转编码器和电源按键。
- `onRender()`：绘制电池电压面板、菜单卡片、图标和当前选中项文字。
- `onPress()`：设置按下动画。
- `onClick()`：设置打开动画。
- `onOpenEnd()`：动画结束后调用对应功能。

菜单项的文字、颜色和图标由 `_app_render_props_list` 统一配置。旋转编码器位置变化时调用 `goLast()` 或 `goNext()` 切换选项；电源按键按下后触发菜单项打开动画，动画结束时 `_open_app()` 根据选中索引调用 `FactoryTest` 中的功能函数。

屏幕绘制采用双缓冲方式：代码先画到 `LGFX_Sprite` 类型的 `_canvas`，再调用 `_canvas_update()` 将 Sprite 推送到屏幕，减少闪烁。

### 硬件抽象

`FactoryTest` 类在 `src/factory_test/factory_test.h` 中集中持有硬件对象和功能入口：

- `_disp`：LovyanGFX 屏幕设备对象。
- `_canvas`：LovyanGFX Sprite 离屏画布。
- `_btn_pwr`：电源按键对象，使用本地 `Button` 库做去抖。
- `_enc`：旋转编码器对象，使用 `ESP32Encoder`。
- `_rtc`：BM8563 RTC 对象。
- `_wifi_list`：Wi-Fi 扫描结果缓存。

这种写法让 UI 层只需要拿到 `FactoryTest*`，就可以调用各硬件测试函数，不需要直接管理底层引脚和外设。

### 屏幕驱动

屏幕配置在 `src/factory_test/components/ft_disp_lgfx_cfg.hpp` 中。项目自定义了 `LGFX_DinMeter`，继承自 `lgfx::LGFX_Device`，配置内容包括：

- SPI 总线引脚：MOSI 5、SCLK 6、DC 4。
- ST7789 面板：CS 7、RST 8、分辨率 135 x 240。
- 面板偏移：`offset_x = 52`，`offset_y = 40`。
- 背光 PWM：BL 9，PWM 通道 7，频率 200 Hz。

`_disp_init()` 中创建 `LGFX_DinMeter` 并设置旋转方向为 `3`，然后创建和屏幕同尺寸的 `LGFX_Sprite` 作为画布。

### 输入与电源

电源按键连接在 GPIO 42，使用 `Button(42, 20)` 初始化，20 ms 去抖。

`_check_next()` 是很多页面的通用退出/确认逻辑：

- 短按电源键：返回 `true`，当前功能页通常据此退出。
- 长按电源键：屏幕提示松开按键关机，松开后调用 `_power_off()`。

`_power_on()` 通过 GPIO 46 保持设备上电；`_power_off()` 清理 RTC IRQ、关闭屏幕显示，等待按键释放后拉低 GPIO 46。

旋转编码器连接在 GPIO 40 和 GPIO 41，使用 `attachHalfQuad()` 读取半步正交编码信号。`_check_encoder()` 比较当前计数和上一次 `_enc_pos`，如果变化则更新计数，并可根据方向播放不同频率的蜂鸣提示。

### 功能模块

各功能在 `src/factory_test/components` 中拆分实现：

- `ft_disp_test.cpp`：显示测试和亮度调节。显示测试会绘制颜色渐变；亮度调节页面用编码器以 5 为步进调整 `setBrightness()`。
- `ft_rtc_test.cpp`：初始化 BM8563，读取时间并每 500 ms 刷新显示；睡眠唤醒功能通过 `SetAlarmIRQ()` 设置 RTC 闹钟。
- `ft_wifi_test.cpp`：用户模式下扫描 Wi-Fi，最多显示 6 个网络的 RSSI、SSID 和加密方式；工厂测试模式下还能连接指定热点并发起 HTTP GET 验证联网能力。
- `ft_ble_test.cpp`：创建 BLE Server，生成基于芯片 EFuse MAC 的设备名，建立 RX/TX Characteristic，连接后周期性 notify 数据。
- `ft_io_test.cpp`：读取电池电压，并在 Grove 1 和 Grove 2 对应引脚上扫描 I2C 地址。
- `ft_arkanoid.cpp`：实现打砖块小游戏，包括玩家挡板、球、砖块、碰撞检测、生命值和绘制逻辑。

## 用了什么库

### PlatformIO 依赖

`platformio.ini` 中配置的远程依赖：

- `lovyan03/LovyanGFX @ 1.1.12`：屏幕驱动、绘图、Sprite 双缓冲、字体和背光控制。
- `forairaaaaa/SmoothUIToolKit @ 1.0.1`：菜单、过渡动画、缓动路径和输入/渲染调度。

构建环境：

- `platform = espressif32@6.3.1`
- `board = esp32-s3-devkitc-1`
- `framework = arduino`
- `board_build.f_cpu = 240000000L`
- `monitor_speed = 115200`
- `monitor_filters = esp32_exception_decoder`

### 项目本地库

`lib` 目录中包含的本地库：

- `Button`：简单按键去抖库，提供 `read()`、`pressed()`、`released()` 等接口。
- `I2C_BM8563`：BM8563 RTC 驱动，项目用于读取时间、清除/关闭 IRQ、设置闹钟唤醒。
- `ESP32Encoder`：ESP32 旋转编码器库，项目用于读取 GPIO 40/41 上的编码器计数。

### Arduino/ESP32 内置库

项目还使用了 ESP32 Arduino 生态中的常见库：

- `WiFi.h`、`esp_wifi.h`：Wi-Fi 扫描、连接、断开和底层 Wi-Fi 资源释放。
- `HTTPClient.h`：工厂 Wi-Fi 测试中的 HTTP GET。
- `BLEDevice.h`、`BLEServer.h`、`BLEUtils.h`、`BLE2902.h`：BLE Server、Service、Characteristic 和 notify descriptor。
- `Wire` / `Wire1`：I2C 总线，分别用于 RTC 和 Grove 接口扫描。
- Arduino API：`tone()`、`noTone()`、`analogReadMilliVolts()`、`millis()`、`delay()`、`pinMode()`、`digitalWrite()` 等。

## 代码讲解

### `src/main.cpp`

这个文件非常薄，只负责串起初始化和主循环：

```cpp
static FactoryTest ft;

void setup()
{
    ft.init();
    view_create(&ft);
}

void loop() { view_update(); }
```

`FactoryTest` 管硬件和业务功能，`view_create()` / `view_update()` 管 UI。主循环不直接写业务逻辑，便于把硬件测试和界面渲染分开。

### `src/factory_test/factory_test.h`

这是项目的核心接口文件。`FactoryTest` 把系统电源、显示、按键、编码器、蜂鸣器、RTC、Wi-Fi、BLE、IO 和小游戏入口全部声明在一个类里。

比较重要的成员包括：

- `_canvas_update()`：把离屏画布刷新到物理屏幕。
- `_check_next()`：统一处理短按确认/退出和长按关机。
- `_check_encoder()`：统一处理编码器计数变化和蜂鸣反馈。
- `_disp_test()`、`_rtc_test()`、`_wifi_test()` 等：被菜单直接调用的功能页面。

### `src/factory_test/factory_test.cpp`

`FactoryTest::init()` 是硬件初始化入口。当前代码中工厂测试自动进入逻辑被注释了：

```cpp
// if (_check_test_mode())
// {
//     start_factory_test();
// }
```

因此正常启动后进入用户 Demo 菜单。如果恢复这段逻辑，长按进入测试模式后可以调用 `start_factory_test()` 按顺序执行工厂测试流程。

### `src/view/view.cpp`

这个文件实现用户可见的主菜单。

`_app_render_props_list` 把每个功能项的主题色、标签颜色、标题和图标绑定在一起。`view_create()` 创建菜单选项并配置关键帧位置：

- 第一个选项是当前选中项，大卡片位置为 `{6, 6, 228, 64}`。
- 其他选项排在下方等待区。
- 最后一项额外放在选中项右侧，用于循环滚动时保持动画连续。

`onRender()` 每帧绘制背景、电池电压面板、菜单卡片、图标和选中标题。电池电压通过 `analogReadMilliVolts(10) * 2 / 1000` 估算，说明硬件上电池检测分压系数按 2 倍处理。

`_open_app()` 是菜单索引到功能函数的映射。例如：

- 索引 0 调用 `_disp_test()`。
- 索引 1 调用 `_disp_set_brightness()`。
- 索引 3 调用 `_wifi_test()`。
- 索引 5 调用 `_arkanoid_start()`。
- 索引 7 调用 `_power_off()`。

### `ft_disp_test.cpp`

显示模块分为初始化、显示测试和亮度设置。

`_disp_init()` 创建屏幕对象、初始化屏幕、设置横屏方向，并创建 Sprite。后续所有页面基本都先画 `_canvas`，再调用 `_canvas_update()`。

`_disp_set_brightness()` 用编码器调整亮度值，范围限制在 0 到 255。用户按下电源键退出页面，长按则进入关机流程。

### `ft_key_test.cpp`

这个文件同时处理按键、电源保持和编码器通用读取逻辑。

`_check_next()` 的设计很关键，因为它被多个功能页复用。短按是“下一步/退出”，长按超过约 1 秒后进入关机提示并执行 `_power_off()`。

`_check_encoder()` 比较编码器新旧位置，位置变化时更新 `_enc_pos`，并用 `tone()` 播放 3000 Hz 或 3500 Hz 的短提示音区分方向。

### `ft_rtc_test.cpp`

RTC 使用 `Wire.begin(11, 12, 100000)` 初始化 I2C。`_rtc_test()` 读取 `I2C_BM8563_TimeTypeDef` 并显示 `HH:MM:SS`。

`_rtc_wakeup_test_user()` 允许用户用编码器选择 5 到 20 秒的唤醒时间，确认后调用 `_rtc.SetAlarmIRQ()` 设置闹钟，再拉低电源保持引脚，让设备等待 RTC 唤醒。

### `ft_wifi_test.cpp`

用户模式下，`_wifi_test()` 做一次 Wi-Fi 扫描：

1. 设置 `WiFi.mode(WIFI_STA)`。
2. 调用 `WiFi.scanNetworks()`。
3. 读取前 6 个网络的 RSSI、SSID 和加密类型。
4. 显示扫描结果。
5. 删除扫描结果并释放 Wi-Fi 资源。

工厂测试模式下，`task_wifi()` 会先扫描并检查最强 RSSI 是否高于 `-65 dBm`，再尝试连接固定热点 `UDUDLRLRBABA`，最后访问 `http://example.com/index.html` 验证 HTTP 通路。

### `ft_ble_test.cpp`

BLE 测试会创建一个 GATT Server：

- Service UUID：`1bc68b2a-f3e3-11e9-81b4-2a2ae2dbcce4`
- Notify Characteristic：`1bc68da0-f3e3-11e9-81b4-2a2ae2dbcce4`
- Write Characteristic：`1bc68efe-f3e3-11e9-81b4-2a2ae2dbcce4`

设备名由 `ESP.getEfuseMac()` 生成，格式类似 `M5-xxxx`。连接后，页面会显示发送状态，并周期性通过 notify 发出递增数据。

### `ft_io_test.cpp`

IO 测试主要验证三类硬件：

- 电池电压：读取 GPIO 10 的毫伏值并乘以 2。
- Grove 1：使用 `Wire1.begin(2, 1, 100000)` 扫描 I2C 设备。
- Grove 2：使用 `Wire1.begin(13, 15, 100000)` 扫描 I2C 设备。

扫描到设备时显示 I2C 地址，否则显示无设备。

### `ft_arkanoid.cpp`

打砖块小游戏是纯本地逻辑实现，没有引入游戏引擎。它定义了 `Player`、`Ball`、`Brick` 和简单的 `Vector2` 结构。

游戏循环由 `_arkanoid_loop()` 控制，每约 15 ms 更新一帧。编码器改变挡板位置，按下电源键发球。球与墙、挡板和砖块的碰撞会反转速度并播放蜂鸣音；生命耗尽或砖块清空后重置游戏。

## 如何构建和烧录

项目使用 PlatformIO。常见命令如下：

```bash
pio run
pio run --target upload
pio device monitor
```

默认环境是 `esp32-s3-devkitc-1`。如果实际 M5 DinMeter 的板级参数或上传端口不同，需要根据硬件连接修改 `platformio.ini` 或在命令中指定端口。

## 设计特点

- UI 和硬件功能分层：`view.cpp` 管交互和动画，`FactoryTest` 管硬件能力。
- 使用 Sprite 双缓冲绘制：降低屏幕刷新闪烁。
- 多个页面复用统一输入语义：编码器调整/切换，电源键确认/退出，长按关机。
- 保留工厂测试能力：同一套硬件接口既能做用户 Demo，也能做产测验证。
- 资源本地化：图标和图片转成头文件数组，固件不依赖外部文件系统。
