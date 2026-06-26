/**
 * @file SleepWakeupApp.h
 * @brief Set RTC alarm then enter deep sleep
 */
#pragma once
#include "AppBase.h"

class SleepWakeupApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        auto& canvas = hw.display.canvas;
        canvas->setFont(&fonts::Font0);
        canvas->fillScreen((uint32_t)0xC6D5EF);

        hw.input.resetEncoder(5);

        char buf[24];
        while (1)
        {
            canvas->fillScreen((uint32_t)0xC6D5EF);

            canvas->fillRect(0, 0, 240, 25, (uint32_t)0x46556F);
            canvas->setTextSize(2);
            canvas->setTextColor((uint32_t)0xC6D5EF);
            snprintf(buf, 20, "Sleep & Wake Up");
            canvas->drawCenterString(buf, canvas->width() / 2, 5);

            canvas->setTextColor((uint32_t)0x46556F);
            canvas->setCursor(0, 30);
            canvas->printf(" Press Button Sleep\n Wake Up In:");

            canvas->setTextSize(5);
            canvas->setTextColor((uint32_t)0x46556F);
            snprintf(buf, 20, "%ds", hw.input.enc_pos);
            canvas->drawCenterString(buf, canvas->width() / 2, 85);

            hw.display.push();

            hw.input.checkEncoder();

            // Clamp
            if (hw.input.enc.getPosition() > 20) hw.input.enc.setPosition(20);
            if (hw.input.enc.getPosition() < 5)  hw.input.enc.setPosition(5);

            if (hw.input.checkNext()) break;
        }

        // Set alarm and sleep
        hw.rtc.clearIRQ();
        hw.rtc.setAlarmIRQ(hw.input.enc.getPosition());
        canvas->fillScreen(TFT_BLACK);
        hw.display.push();
        delay(500);

        hw.power.cut();
    }
};
