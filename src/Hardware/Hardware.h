/**
 * @file Hardware.h
 * @brief Hardware facade — owns all peripherals, provides init sequence
 */
#pragma once
#include "Display.h"
#include "Input.h"
#include "RTC.h"
#include "Buzzer.h"
#include "Power.h"

class Hardware
{
public:
    Display display;
    Input   input;
    RTC     rtc;
    Buzzer  buzzer;
    Power   power;

    void init()
    {
        power.hold();
        rtc.init();
        input.init();
        display.init();
    }
};
