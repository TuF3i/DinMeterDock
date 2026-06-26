/**
 * @file BrightnessApp.h
 * @brief Display brightness control via encoder
 */
#pragma once
#include "AppBase.h"

class BrightnessApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        auto& canvas = hw.display.canvas;

        canvas->setFont(&fonts::Font0);

        int brightness = hw.display.getBrightness();
        long old_pos = hw.input.enc_pos;
        char buf[20];

        hw.input.resetEncoder(0);

        while (1)
        {
            canvas->fillScreen((uint32_t)0x87C38F);

            canvas->fillRect(0, 0, 240, 25, (uint32_t)0x07430F);
            canvas->setTextSize(2);
            canvas->setTextColor((uint32_t)0x87C38F);
            snprintf(buf, 20, "Set Brightness");
            canvas->drawCenterString(buf, canvas->width() / 2, 5);

            canvas->setTextSize(5);
            canvas->setTextColor((uint32_t)0x07430F);
            snprintf(buf, 20, "%d", brightness);
            canvas->drawCenterString(buf, canvas->width() / 2, 55);

            hw.display.push();

            if (hw.input.checkEncoder())
            {
                if (hw.input.enc_pos > old_pos)
                    brightness += 5;
                else
                    brightness -= 5;

                if (brightness > 255) brightness = 255;
                else if (brightness < 0) brightness = 0;

                old_pos = hw.input.enc_pos;
                hw.display.setBrightness(brightness);
            }

            if (hw.input.checkNext()) break;
        }
    }
};
