/**
 * @file PowerOffApp.h
 * @brief Power off the device
 */
#pragma once
#include "AppBase.h"

class PowerOffApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        hw.rtc.clearIRQ();
        hw.rtc.disableIRQ();

        hw.display.fillScreen(TFT_BLACK);
        hw.power.off();
    }
};
