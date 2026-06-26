/**
 * @file Launcher.h
 * @brief Menu launcher UI with smooth animations
 */
#pragma once
#include "../Hardware/Hardware.h"
#include "../Apps/AppBase.h"
#include "Assets/assets.h"
#include <functional>

class Launcher
{
public:
    Launcher(Hardware& hw);
    void begin();
    void update();

private:
    Hardware& _hw;

    // --- App registry entry ---
    struct AppEntry
    {
        const char*       name;
        uint32_t          themeColor;
        uint32_t          tagColor;
        const uint16_t*   icon;
        std::function<void(Hardware&)> runner;
    };

    static constexpr int APP_COUNT = 8;
    const AppEntry _apps[APP_COUNT];

    // --- Internals (implemented in Launcher.cpp) ---
    struct Impl;
    Impl* _impl;

    void _openApp(int index);
};
