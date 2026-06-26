/**
 * @file DisplayApp.h
 * @brief Display color gradient test
 */
#pragma once
#include "AppBase.h"

class DisplayApp : public AppBase
{
public:
    void run(Hardware& hw) override
    {
        auto& canvas = hw.display.canvas;

        float r = 0, g = 0, b = 255;

        for (int i = 0; i < 384; i += 4)
        {
            if (i < 128)           { r = i * 2;       g = 0;             b = 255 - (i * 2); }
            else if (i < 256)      { r = 255 - ((i - 128) * 2); g = (i - 128) * 2; b = 0; }
            else                   { r = 0;           g = 255 - ((i - 256) * 2); b = (i - 256) * 2; }
            canvas->fillRect(0, 0, 240, 135, canvas->color565(r, g, b));
            hw.display.push();
        }

        for (int i = 0; i < 4; i++)
        {
            switch (i)
            {
            case 0: r = 0;   g = 0;   b = 0;   break;
            case 1: r = 255; g = 0;   b = 0;   break;
            case 2: r = 0;   g = 255; b = 0;   break;
            case 3: r = 0;   g = 0;   b = 255; break;
            }
            for (int n = 0; n < 240; n++)
            {
                r = (r < 255) ? r + 1.0625 : 255U;
                g = (g < 255) ? g + 1.0625 : 255U;
                b = (b < 255) ? b + 1.0625 : 255U;
                canvas->drawLine(n, i * 33.75, n, (i + 1) * 33.75, canvas->color565(r, g, b));
            }
        }
        hw.display.push();
        delay(500);

        for (int i = 0; i < 4; i++)
        {
            switch (i)
            {
            case 0: r = 255; g = 255; b = 255; break;
            case 1: r = 255; g = 0;   b = 0;   break;
            case 2: r = 0;   g = 255; b = 0;   break;
            case 3: r = 0;   g = 0;   b = 255; break;
            }
            for (int n = 0; n < 240; n++)
            {
                r = (r > 2) ? r - 1.0625 : 0U;
                g = (g > 2) ? g - 1.0625 : 0U;
                b = (b > 2) ? b - 1.0625 : 0U;
                canvas->drawLine(239 - n, i * 33.75, 239 - n, (i + 1) * 33.75, canvas->color565(r, g, b));
            }
        }
        hw.display.push();
        delay(500);

        hw.input.waitNext();
    }
};
