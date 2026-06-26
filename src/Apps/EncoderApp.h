/**
 * @file EncoderApp.h
 * @brief Rotary encoder counter display
 */
#pragma once
#include "AppBase.h"

class EncoderApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        auto& canvas = hw.display.canvas;
        canvas->setFont(&fonts::Font0);

        char buf[20];
        hw.input.resetEncoder(0);

        while (1)
        {
            canvas->fillScreen((uint32_t)0x6AB8A0);

            canvas->fillRect(0, 0, 240, 25, (uint32_t)0x163820);
            canvas->setTextSize(2);
            canvas->setTextColor((uint32_t)0x6AB8A0);
            snprintf(buf, 20, "Encoder Test");
            canvas->drawCenterString(buf, canvas->width() / 2, 5);

            canvas->setTextSize(5);
            canvas->setTextColor((uint32_t)0x163820);
            snprintf(buf, 20, "%d", hw.input.enc_pos);
            canvas->drawCenterString(buf, canvas->width() / 2, 55);

            hw.display.push();

            hw.input.checkEncoder();
            if (hw.input.checkNext()) break;
        }
    }
};
