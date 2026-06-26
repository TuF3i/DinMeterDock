/**
 * @file Display.h
 * @brief LGFX display configuration and wrapper for M5DinMeter
 */
#pragma once
#include <LovyanGFX.h>
#include "../Config.h"

// --- LGFX_DinMeter: ST7789 panel over SPI ---
class LGFX_DinMeter : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI      _bus_instance;
    lgfx::Light_PWM    _light_instance;

public:
    LGFX_DinMeter()
    {
        {
            auto cfg = _bus_instance.config();
            cfg.pin_mosi = LCD_MOSI_PIN;
            cfg.pin_miso = LCD_MISO_PIN;
            cfg.pin_sclk = LCD_SCLK_PIN;
            cfg.pin_dc   = LCD_DC_PIN;
            cfg.freq_write = 40000000;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.invert      = true;
            cfg.pin_cs      = LCD_CS_PIN;
            cfg.pin_rst     = LCD_RST_PIN;
            cfg.pin_busy    = LCD_BUSY_PIN;
            cfg.panel_width  = LCD_PANEL_WIDTH;
            cfg.panel_height = LCD_PANEL_HEIGHT;
            cfg.offset_x    = LCD_OFFSET_X;
            cfg.offset_y    = LCD_OFFSET_Y;
            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = LCD_BL_PIN;
            cfg.invert = false;
            cfg.freq   = 200;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }
        setPanel(&_panel_instance);
    }
};

// --- Display wrapper ---
class Display
{
public:
    LGFX_Sprite* canvas = nullptr;

    void init()
    {
        _disp = new LGFX_DinMeter;
        _disp->init();
        _disp->setRotation(3);

        canvas = new LGFX_Sprite(_disp);
        canvas->createSprite(_disp->width(), _disp->height());
    }

    inline void push() { canvas->pushSprite(0, 0); }

    inline void setBrightness(int v) { _disp->setBrightness(v); }
    inline int  getBrightness()      { return _disp->getBrightness(); }

    inline void fillScreen(uint32_t color) { _disp->fillScreen(color); }

    LGFX_Device* device() { return _disp; }

private:
    LGFX_DinMeter* _disp = nullptr;
};
