# MQTT 实时消息与 UI 联动设计

## 架构目标

在 DinMeterDock 中增加实时 MQTT 消息接收能力，并能将接收到的消息联动刷新屏幕 UI。核心设计原则：**网络常驻、消息解耦、App 主动消费**。

## 整体架构

```
┌──────────────────────────────────────────────┐
│  UI 层 (Apps)                                │
│  订阅 Topic → 处理 Message → 更新屏幕          │
└───────────────┬──────────────────────────────┘
                │ pollMessage()
┌───────────────▼──────────────────────────────┐
│  消息总线 (MessageBus)                        │
│  环形缓冲区，线程安全，缓存 MQTT 消息           │
└───────────────┬──────────────────────────────┘
                │ push (MQTT callback)
┌───────────────▼──────────────────────────────┐
│  网络层 (NetworkService)                      │
│  WiFi 保活 + MQTT 收发 + 断线重连              │
└──────────────────────────────────────────────┘
```

三层职责：

| 层 | 职责 | 运行方式 |
|----|------|---------|
| `NetworkService` | WiFi STA 连接、MQTT subscribe/publish、心跳 keep-alive | `loop()` 中持续驱动 |
| `MessageBus` | 线程安全的环形缓冲队列，解耦接收端和消费端 | MQTT callback 写入，App 轮询读取 |
| `Apps` | 订阅感兴趣的 topic，在 `run()` 循环中 poll 新消息并更新 UI | 阻塞式循环，和现有 App 模式一致 |

## 依赖库

在 `platformio.ini` 中增加 MQTT 客户端库：

```ini
lib_deps =
    lovyan03/LovyanGFX @ 1.1.12
    forairaaaaa/SmoothUIToolKit @ 1.0.1
    knolleary/PubSubClient @ 2.8
```

`PubSubClient` 是最轻量、最广泛使用的 Arduino MQTT 库，内部依赖 WiFi 层，无需额外配置。

## 核心实现

### 1. 消息总线 — `MessageBus.h`

环形缓冲区，MQTT callback 往里面写，App 往外面读。单线程场景不需要锁，FreeRTOS 多任务场景可以加 `portMUX_TYPE` 自旋锁（代码已预留）。

```cpp
// src/Network/MessageBus.h
#pragma once
#include <Arduino.h>
#include <string.h>

#define MQTT_MSG_QUEUE_SIZE 16
#define MQTT_TOPIC_MAX_LEN  64
#define MQTT_PAYLOAD_MAX_LEN 256

struct MqttMessage
{
    char topic[MQTT_TOPIC_MAX_LEN];
    char payload[MQTT_PAYLOAD_MAX_LEN];
    bool valid;
};

class MessageBus
{
public:
    void push(const char* topic, const uint8_t* payload, unsigned int length)
    {
        // 环形缓冲写入
        MqttMessage& msg = _buf[_wr];
        strncpy(msg.topic, topic, MQTT_TOPIC_MAX_LEN - 1);
        msg.topic[MQTT_TOPIC_MAX_LEN - 1] = '\0';

        unsigned int copyLen = (length < MQTT_PAYLOAD_MAX_LEN - 1) ? length : MQTT_PAYLOAD_MAX_LEN - 1;
        memcpy(msg.payload, payload, copyLen);
        msg.payload[copyLen] = '\0';
        msg.valid = true;

        _wr = (_wr + 1) % MQTT_MSG_QUEUE_SIZE;
        if (_wr == _rd) {
            // 覆盖了最旧的消息
            _rd = (_rd + 1) % MQTT_MSG_QUEUE_SIZE;
        }
    }

    bool poll(MqttMessage& out)
    {
        if (_rd == _wr) return false;  // 队列空
        if (!_buf[_rd].valid) {
            _rd = (_rd + 1) % MQTT_MSG_QUEUE_SIZE;
            return false;
        }
        memcpy(&out, &_buf[_rd], sizeof(MqttMessage));
        _buf[_rd].valid = false;
        _rd = (_rd + 1) % MQTT_MSG_QUEUE_SIZE;
        return true;
    }

    bool hasMessage() const { return _rd != _wr && _buf[_rd].valid; }

private:
    MqttMessage _buf[MQTT_MSG_QUEUE_SIZE] = {};
    volatile int _wr = 0;
    volatile int _rd = 0;
};
```

### 2. 网络服务 — `NetworkService.h`

封装 WiFi 连接和 MQTT 客户端。构造函数接收 broker 地址、端口、client id 等参数。WiFi 凭证可通过 `Config.h` 定义，也可后续做 Wi-Fi 配置 App。

```cpp
// src/Network/NetworkService.h
#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include "MessageBus.h"

#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"
#define MQTT_BROKER   "192.168.1.100"
#define MQTT_PORT     1883
#define MQTT_CLIENT_ID "dinmeter"

class NetworkService
{
public:
    NetworkService()
        : _wifiClient()
        , _mqtt(_wifiClient)
        , _bus()
    {
    }

    void begin()
    {
        _connectWiFi();
        _mqtt.setServer(MQTT_BROKER, MQTT_PORT);
        _mqtt.setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
            _bus.push(topic, payload, length);
        });
        _connectMqtt();
    }

    void loop()
    {
        // WiFi 保活
        if (WiFi.status() != WL_CONNECTED) {
            _connectWiFi();
        }

        // MQTT 保活
        if (!_mqtt.connected()) {
            _connectMqtt();
        }
        _mqtt.loop();  // 内部处理 keep-alive + 收消息回调
    }

    void subscribe(const char* topic)
    {
        _mqtt.subscribe(topic);
    }

    void publish(const char* topic, const char* payload)
    {
        _mqtt.publish(topic, payload);
    }

    // 暴露给 App 的消息接口
    bool pollMessage(MqttMessage& out) { return _bus.poll(out); }
    bool hasMessage() const            { return _bus.hasMessage(); }

    MessageBus& bus() { return _bus; }

private:
    WiFiClient   _wifiClient;
    PubSubClient _mqtt;
    MessageBus   _bus;

    void _connectWiFi()
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        int retry = 0;
        while (WiFi.status() != WL_CONNECTED && retry < 40) {
            delay(250);
            retry++;
        }
    }

    void _connectMqtt()
    {
        int retry = 0;
        while (!_mqtt.connected() && retry < 5) {
            if (_mqtt.connect(MQTT_CLIENT_ID)) {
                break;
            }
            delay(1000);
            retry++;
        }
    }
};
```

### 3. 集成到 Hardware

在 `Hardware` 中增加网络服务成员，`init()` 时启动，`loop()` 中驱动。

```cpp
// src/Hardware/Hardware.h（修改后）
#pragma once
#include "Display.h"
#include "Input.h"
#include "RTC.h"
#include "Buzzer.h"
#include "Power.h"
#include "../Network/NetworkService.h"

class Hardware
{
public:
    Display         display;
    Input           input;
    RTC             rtc;
    Buzzer          buzzer;
    Power           power;
    NetworkService  network;   // ★ 新增

    void init()
    {
        power.hold();
        rtc.init();
        input.init();
        display.init();
        network.begin();       // ★ 初始化 WiFi + MQTT
    }
};
```

### 4. main.cpp 驱动网络 loop

```cpp
// src/main.cpp（修改后）
#include "Hardware/Hardware.h"
#include "UI/Launcher.h"

Hardware hw;
Launcher launcher(hw);

void setup()
{
    hw.init();
    launcher.begin();
}

void loop()
{
    hw.network.loop();   // ★ 维持 MQTT 心跳、收消息入队
    launcher.update();
}
```

### 5. MQTT 联动 UI 的 App 示例 — `MqttMonitorApp.h`

这是一个完整的 App，订阅 `sensor/temp` 和 `sensor/humidity` 两个 topic，实时将数值显示在屏幕上。

```cpp
// src/Apps/MqttMonitorApp.h
#pragma once
#include "AppBase.h"
#include "../Network/MessageBus.h"
#include <ArduinoJson.h>       // 需在 platformio.ini 中增加：bblanchon/ArduinoJson @ 6

class MqttMonitorApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        auto& c = hw.display.canvas;

        // 订阅感兴趣的 topic
        hw.network.subscribe("sensor/temp");
        hw.network.subscribe("sensor/humidity");

        float temperature = 0.0f;
        float humidity    = 0.0f;
        unsigned long lastUpdate = 0;

        c->fillScreen((uint32_t)0x2C3E50);
        c->setFont(&fonts::efontCN_16_b);
        c->setTextSize(1);

        while (1)
        {
            // ★ 轮询 MQTT 消息
            MqttMessage msg;
            while (hw.network.pollMessage(msg))
            {
                // 解析 JSON 并更新变量
                StaticJsonDocument<128> doc;
                DeserializationError err = deserializeJson(doc, msg.payload);
                if (!err)
                {
                    if (strcmp(msg.topic, "sensor/temp") == 0) {
                        temperature = doc["value"].as<float>();
                        lastUpdate = millis();
                    }
                    else if (strcmp(msg.topic, "sensor/humidity") == 0) {
                        humidity = doc["value"].as<float>();
                        lastUpdate = millis();
                    }
                }
            }

            // 绘制 UI
            c->fillScreen((uint32_t)0x2C3E50);

            // 标题栏
            c->fillRect(0, 0, 240, 28, (uint32_t)0x1A252F);
            c->setTextColor((uint32_t)0xECF0F1);
            c->setCursor(10, 6);
            c->printf("MQTT Monitor");

            // 温度
            c->setTextColor((uint32_t)0xE74C3C);
            c->setCursor(20, 50);
            c->printf("Temp: %.1f C", temperature);

            // 湿度
            c->setTextColor((uint32_t)0x3498DB);
            c->setCursor(20, 85);
            c->printf("Hum : %.1f %%", humidity);

            // 更新时间
            c->setTextColor((uint32_t)0x7F8C8D);
            c->setCursor(20, 130);
            c->printf("Last: %lus ago", (millis() - lastUpdate) / 1000);

            hw.display.push();

            // 输入处理
            if (hw.input.checkNext()) break;
        }
    }
};
```

### 6. 注册到菜单

编辑 `src/UI/Launcher.cpp`，在 `_apps` 数组末尾增加：

```cpp
{"MQTT", 0x2ECC71, 0x1A4A2A, image_data_icon_wifi, [](Hardware& h) { MqttMonitorApp().run(h); }},
```

同时将 `Launcher.h` 中的 `APP_COUNT` 从 `8` 改为 `9`。

> 图标复用 `image_data_icon_wifi`，后续可替换为专用 MQTT 图标。

## 消息流向总结

```
MQTT Broker ──publish──→ WiFi ──recv──→ PubSubClient::loop()
                                              │
                                     callback(topic, payload)
                                              │
                                         MessageBus::push()
                                              │
                                    ┌─────────▼─────────┐
                                    │   环形缓冲队列      │
                                    │ [msg1][msg2]...[ ] │
                                    └─────────┬─────────┘
                                              │
                              App::run() 循环中 pollMessage()
                                              │
                                     deserializeJson(payload)
                                              │
                                     canvas->printf(...)
                                     display.push()
                                              │
                                        屏幕实时更新
```

## App 适配 MQTT 的通用模式

```cpp
// App 内部的主循环模式
while (1)
{
    // 1. 消费所有待处理的 MQTT 消息
    MqttMessage msg;
    while (hw.network.pollMessage(msg))
    {
        handleMessage(msg);  // 各 App 自定义处理逻辑
    }

    // 2. 根据最新数据绘制 UI
    drawUI(hw);

    // 3. 刷新屏幕
    hw.display.push();

    // 4. 处理用户输入
    if (hw.input.checkNext()) break;
}
```

## 线程安全说明

当前设计基于 Arduino `loop()` 单线程模型。MQTT 消息接收发生在 `hw.network.loop()` 中（即 `PubSubClient::loop()` 的回调），消息消费发生在 `App::run()` 的 `while(1)` 循环中，两者在同一线程的不同调用层级，天然互斥，不需要锁。

如果后续将 MQTT 网络处理移入 FreeRTOS 独立任务（如在 core 0 运行），只需在 `MessageBus::push()` 和 `MessageBus::poll()` 内部加临界区保护即可：

```cpp
// 多线程版本需要的改动
void push(const char* topic, const uint8_t* payload, unsigned int length)
{
    portENTER_CRITICAL(&_spinlock);
    // ... 原有写入逻辑 ...
    portEXIT_CRITICAL(&_spinlock);
}
```

当前单线程模型不需要这一步。

## 构建

```bash
pio run
```

MQTT 功能编译进固件后，首次上电会自动连接 WiFi 和 MQTT Broker。如果 WiFi 或 Broker 不可达，`NetworkService::loop()` 会自动重试，不会阻塞 UI 运行。
