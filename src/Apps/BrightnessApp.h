/**
 * @file BrightnessApp.h
 * @brief Display brightness control via encoder with visual feedback
 */
#pragma once
#include "AppBase.h"

class BrightnessApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        auto& c = hw.display.canvas;

        // Read current brightness
        int brightness = hw.display.getBrightness();
        if (brightness < 0)   brightness = 0;
        if (brightness > 255) brightness = 255;

        // Reset encoder to track relative changes
        hw.input.resetEncoder(0);
        int oldPos = 0;

        while (1)
        {
            // Silent encoder check (no buzzer during adjustment for smoothness)
            if (hw.input.checkEncoder(false))
            {
                if (hw.input.enc_pos > oldPos)
                    brightness += 5;
                else
                    brightness -= 5;

                if (brightness < 0)   brightness = 0;
                if (brightness > 255) brightness = 255;

                oldPos = hw.input.enc_pos;
                hw.display.setBrightness(brightness);
            }

            // --- Render ---
            c->fillScreen(TFT_WHITE);

            // Title bar
            c->fillRect(0, 0, 240, 25, (uint32_t)0x07430F);
            c->setFont(&fonts::efontCN_16);
            c->setTextSize(1);
            c->setTextColor(TFT_WHITE);
            c->setTextDatum(top_center);
            c->drawString("Brightness", 120, 4);

            // Progress bar background
            c->fillRoundRect(40, 55, 160, 20, 4, (uint32_t)0xCCCCCC);

            // Progress bar fill
            int fillW = (int)((long)160 * brightness / 255);
            if (fillW > 0)
                c->fillRoundRect(40, 55, fillW, 20, 4, (uint32_t)0x07430F);

            // Numeric value (large, centered)
            c->setFont(&fonts::efontCN_24);
            c->setTextColor((uint32_t)0x333333);
            c->setTextDatum(middle_center);
            char buf[16];
            snprintf(buf, 16, "%d", brightness);
            c->drawString(buf, 120, 100);

            // Percentage
            c->setFont(&fonts::efontCN_16);
            c->setTextColor((uint32_t)0x888888);
            int pct = (int)((long)brightness * 100 / 255);
            snprintf(buf, 16, "%d%%", pct);
            c->drawString(buf, 120, 120);

            // Scale marks
            c->setFont(&fonts::Font0);
            c->setTextSize(1);
            c->setTextColor((uint32_t)0x888888);
            c->setTextDatum(top_center);
            c->drawString("0", 42, 78);
            c->drawString("128", 120, 78);
            c->drawString("255", 198, 78);

            // Hint
            c->setFont(&fonts::efontCN_16);
            c->setTextColor((uint32_t)0x888888);
            c->setTextDatum(bottom_center);
            c->drawString("Turn: Adjust  |  Click: Save & Back", 120, 130);

            hw.display.push();

            // Exit on button press
            if (hw.input.checkNext(false)) break;
        }
    }
};
