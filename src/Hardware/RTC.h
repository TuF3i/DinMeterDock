/**
 * @file RTC.h
 * @brief BM8563 RTC wrapper
 */
#pragma once
#include <Wire.h>
#include <I2C_BM8563.h>
#include "../Config.h"

class RTC
{
public:
    void init()
    {
        Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN, 100000);
        _rtc.begin();
        _rtc.clearIRQ();
        _rtc.disableIRQ();
    }

    void getTime(I2C_BM8563_TimeTypeDef* t) { _rtc.getTime(t); }

    void setAlarmIRQ(int seconds) { _rtc.SetAlarmIRQ(seconds); }

    void clearIRQ()   { _rtc.clearIRQ(); }
    void disableIRQ() { _rtc.disableIRQ(); }

    I2C_BM8563& raw() { return _rtc; }

private:
    I2C_BM8563 _rtc;
};
