/**
 * @file Buzzer.h
 * @brief Buzzer tone output with configurable volume via LEDC PWM
 */
#pragma once
#include <Arduino.h>
#include "../Config.h"

class Buzzer
{
public:
    /// @brief Play a tone at given frequency and duration.
    /// @param freq     Frequency in Hz (valid range ~40-20000).
    /// @param duration Duration in ms. 0 = play until noTone() is called.
    void tone(unsigned int freq, unsigned long duration = 0)
    {
        _lastFreq = freq;
        ledcSetup(_channel, freq, _resolution);
        ledcAttachPin(BUZZ_PIN, _channel);
        ledcWrite(_channel, _volume);

        if (duration > 0)
        {
            delay(duration);
            noTone();
        }
    }

    /// @brief Stop any playing tone.
    inline void noTone()
    {
        _lastFreq = 0;
        ledcDetachPin(BUZZ_PIN);
    }

    /// @brief Random short click for game effects (respects volume).
    inline void click()
    {
        unsigned int freq = random(200, 600);
        ledcSetup(_channel, freq, _resolution);
        ledcAttachPin(BUZZ_PIN, _channel);
        ledcWrite(_channel, _volume);
        delay(30);
        ledcDetachPin(BUZZ_PIN);
    }

    /// @brief Set global volume.
    /// @param v Volume level (0-255). 0 = silent, 255 = max.
    ///          Default is 128 (50% duty). Updates active tone if playing.
    void setVolume(uint8_t v)
    {
        _volume = v;
        if (_lastFreq > 0)
        {
            ledcWrite(_channel, _volume);
        }
    }

    /// @brief Get current volume setting (0-255).
    inline uint8_t getVolume() const { return _volume; }

private:
    static constexpr uint8_t _channel    = 2;    // LEDC ch 2 on Timer 1 (independent of backlight ch 7 on Timer 3)
    static constexpr uint8_t _resolution = 8;    // 8-bit => 0-255 duty range

    uint8_t       _volume   = 128;     // 50% duty
    unsigned int  _lastFreq = 0;
};
