/**
 * @file BuzzerVolumeApp.h
 * @brief Buzzer volume adjustment (0-255)
 */
#pragma once
#include "AppBase.h"

class BuzzerVolumeApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        auto& c = hw.display.canvas;
        int volume = hw.buzzer.getVolume();

        hw.input.resetEncoder(volume);
        int lastEnc = volume;

        while (1)
        {
            // Read encoder
            hw.input.checkEncoder(false);  // no built-in buzz, we preview ourselves
            if (hw.input.enc_pos != lastEnc)
            {
                // Clamp 0-255, step by 5
                int delta = hw.input.enc_pos - lastEnc;
                volume += delta * 5;
                if (volume < 0)   volume = 0;
                if (volume > 255) volume = 255;
                lastEnc = hw.input.enc_pos;

                hw.buzzer.setVolume(volume);
                // Preview sound
                hw.buzzer.tone(2000, 30);
                hw.buzzer.noTone();
            }

            // Draw
            c->fillScreen(TFT_WHITE);

            // Title bar
            c->fillRect(0, 0, 240, 25, (uint32_t)0x8B6914);
            c->setFont(&fonts::efontCN_16);
            c->setTextSize(1);
            c->setTextColor(TFT_WHITE);
            c->setTextDatum(top_center);
            c->drawString("Buzzer Volume", 120, 4);

            // Volume bar background
            c->fillRoundRect(40, 55, 160, 20, 4, (uint32_t)0xCCCCCC);

            // Volume bar fill
            int fillW = (int)(160 * volume / 255);
            if (fillW > 0)
                c->fillRoundRect(40, 55, fillW, 20, 4, (uint32_t)0x8B6914);

            // Volume value
            c->setFont(&fonts::efontCN_24);
            c->setTextColor((uint32_t)0x333333);
            c->setTextDatum(middle_center);
            char buf[16];
            snprintf(buf, 16, "%d", volume);
            c->drawString(buf, 120, 100);

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

            // Exit on button
            if (hw.input.checkNext()) break;
        }
    }
};
