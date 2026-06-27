/**
 * @file Input.h
 * @brief Button and rotary encoder input handling
 */
#pragma once
#include <Arduino.h>
#include <Button.h>
#include <ESP32Encoder.h>
#include "Buzzer.h"
#include "../Config.h"

class Input
{
public:
    Button        btn_pwr = Button(BTN_PWR_PIN, BTN_DEBOUNCE_MS);
    ESP32Encoder  enc;
    int           enc_pos = 0;
    Buzzer*       _buzzer = nullptr;   // set by Hardware::init() for volume-aware sounds

    void init()
    {
        btn_pwr.begin();
        enc.attachHalfQuad(ENC_PIN_A, ENC_PIN_B);
        enc.setCount(0);
    }

    // --- Encoder ---

    /// @return true if encoder position changed
    bool checkEncoder(bool playBuzz = true)
    {
        if (enc_pos != enc.getPosition())
        {
            if (playBuzz && _buzzer)
            {
                _buzzer->noTone();
                _buzzer->tone((enc.getPosition() > enc_pos) ? 3000 : 3500, 20);
            }
            else if (playBuzz)
            {
                noTone(BUZZ_PIN);
                tone(BUZZ_PIN, (enc.getPosition() > enc_pos) ? 3000 : 3500, 20);
            }
            enc_pos = enc.getPosition();
            return true;
        }
        return false;
    }

    inline void resetEncoder(int pos = 0)
    {
        enc_pos = pos;
        enc.setPosition(pos);
    }

    // --- Button ---

    /// @return true on short press. Long press triggers power-off (does not return).
    bool checkNext(bool checkPowerOff = true)
    {
        if (!btn_pwr.read())
        {
            if (_buzzer)
                _buzzer->tone(2500, 50);
            else
                tone(BUZZ_PIN, 2500, 50);

            uint8_t time_count = 0;
            while (!btn_pwr.read())
            {
                time_count++;
                if (time_count > 100 && checkPowerOff)
                {
                    return false;
                }
                delay(10);
            }
            return true; // short press
        }
        return false;
    }

    /// @brief Block until button pressed (with encoder beep and reboot check)
    void waitNext()
    {
        while (!checkNext()) { checkEncoder(); delay(10); }
    }

private:
    void _checkReboot() {}
};
