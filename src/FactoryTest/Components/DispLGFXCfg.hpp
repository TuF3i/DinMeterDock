/**
 * @file DispLGFXCfg.hpp
 * @brief LGFX display configuration for M5DinMeter
 */
#pragma once
#include <LovyanGFX.h>

#define LCD_MOSI_PIN 5
#define LCD_MISO_PIN -1
#define LCD_SCLK_PIN 6
#define LCD_DC_PIN 4
#define LCD_CS_PIN 7
#define LCD_RST_PIN 8
#define LCD_BUSY_PIN -1
#define LCD_BL_PIN 9

class LGFX_DinMeter : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX_DinMeter(void)
    {
        {
            auto cfg = _bus_instance.config();

            cfg.pin_mosi = LCD_MOSI_PIN;
            cfg.pin_miso = LCD_MISO_PIN;
            cfg.pin_sclk = LCD_SCLK_PIN;
            cfg.pin_dc = LCD_DC_PIN;
            cfg.freq_write = 40000000;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();

            cfg.invert = true;
            cfg.pin_cs = LCD_CS_PIN;
            cfg.pin_rst = LCD_RST_PIN;
            cfg.pin_busy = LCD_BUSY_PIN;
            cfg.panel_width = 135;
            cfg.panel_height = 240;
            cfg.offset_x = 52;
            cfg.offset_y = 40;

            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();

            cfg.pin_bl = LCD_BL_PIN;
            cfg.invert = false;
            cfg.freq = 200;
            cfg.pwm_channel = 7;

            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
    }
};
