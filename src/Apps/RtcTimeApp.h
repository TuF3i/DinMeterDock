/**
 * @file RtcTimeApp.h
 * @brief RTC time display (HH:MM:SS)
 */
#pragma once
#include "AppBase.h"

class RtcTimeApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        auto& canvas = hw.display.canvas;
        canvas->setFont(&fonts::Font0);

        I2C_BM8563_TimeTypeDef time;
        uint32_t time_count = millis();

        while (1)
        {
            if ((millis() - time_count) > 500)
            {
                hw.rtc.getTime(&time);

                canvas->fillScreen((uint32_t)0xC9C9EE);
                canvas->setTextSize(4);
                canvas->setTextColor((uint32_t)0x49496E);
                canvas->setCursor(25, 55);
                canvas->printf("%02d:%02d:%02d", time.hours, time.minutes, time.seconds);
                canvas->fillRect(0, 0, 240, 25, (uint32_t)0x49496E);
                canvas->setTextSize(2);
                canvas->setTextColor((uint32_t)0xC9C9EE);
                canvas->drawCenterString("RTC Time", canvas->width() / 2, 5);

                hw.display.push();
                time_count = millis();
            }

            hw.input.checkEncoder();
            if (hw.input.checkNext()) break;
        }
    }
};
