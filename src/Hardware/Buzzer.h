/**
 * @file Buzzer.h
 * @brief Tone / noTone wrapper
 */
#pragma once
#include <Arduino.h>
#include "../Config.h"

class Buzzer
{
public:
    inline void tone(unsigned int freq, unsigned long duration = 0) { ::tone(BUZZ_PIN, freq, duration); }
    inline void noTone() { ::noTone(BUZZ_PIN); }

    /// @brief Random short click for game effects
    inline void click() { ::tone(BUZZ_PIN, random(200, 600), 30); }
};
