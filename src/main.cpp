/**
 * @file main.cpp
 * @brief Entry point — initializes hardware then enters the launcher menu
 */
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
    launcher.update();
}
