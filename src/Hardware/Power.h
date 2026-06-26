/**
 * @file Power.h
 * @brief Power hold/off management
 */
#pragma once
#include <Arduino.h>
#include "../Config.h"

class Power
{
public:
    void hold()
    {
        gpio_reset_pin((gpio_num_t)POWER_HOLD_PIN);
        pinMode(POWER_HOLD_PIN, OUTPUT);
        digitalWrite(POWER_HOLD_PIN, HIGH);
    }

    /// @brief Full power-off sequence. Blocks until button released, then cuts power.
    void off()
    {
        // Clear RTC interrupts
        // rtc.clearIRQ() / rtc.disableIRQ() should be called before this

        // Wait for button release
        while (digitalRead(BTN_PWR_PIN) == LOW) { delay(100); }
        delay(200);

        digitalWrite(POWER_HOLD_PIN, LOW);
        delay(10000);

        while (1) { delay(1000); }
    }

    /// @brief Cut power hold immediately (for sleep/wake)
    inline void cut()
    {
        digitalWrite(POWER_HOLD_PIN, LOW);
        while (1) { delay(50); }
    }
};
