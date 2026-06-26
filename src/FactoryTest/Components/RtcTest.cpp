/**
 * @file RtcTest.cpp
 * @brief BM8563 RTC initialization, time display, and wakeup test
 */
#include "../FactoryTest.h"

void FactoryTest::_rtc_init()
{
    Wire.begin(11, 12, 100000);

    _rtc.begin();
    _rtc.clearIRQ();
    _rtc.disableIRQ();
}

void FactoryTest::_rtc_test()
{
    printf("rtc test\n");

    _canvas->setFont(&fonts::Font0);

    I2C_BM8563_TimeTypeDef time;
    _rtc.getTime(&time);

    uint32_t time_count = millis();

    while (1)
    {
        if ((millis() - time_count) > 500)
        {
            _rtc.getTime(&time);

            _canvas->fillScreen((uint32_t)0xC9C9EE);
            _canvas->setTextSize(4);
            _canvas->setTextColor((uint32_t)0x49496E);
            _canvas->setCursor(25, 55);
            _canvas->printf("%02d:%02d:%02d", time.hours, time.minutes, time.seconds);
            _canvas->fillRect(0, 0, 240, 25, (uint32_t)0x49496E);
            _canvas->setTextSize(2);
            _canvas->setTextColor((uint32_t)0xC9C9EE);
            _canvas->drawCenterString("RTC Time", _canvas->width() / 2, 5);

            if (_is_test_mode)
            {
                _canvas->setFont(&fonts::efontCN_24);
                _canvas->setTextColor(TFT_YELLOW, TFT_BLACK);
                _canvas->setTextSize(1);
                _canvas->setCursor(0, 100);
                _canvas->printf(" Verify value is valid and changing");
                _canvas->setFont(&fonts::Font0);
            }

            _canvas_update();

            time_count = millis();
        }

        _check_encoder();
        if (_check_next())
        {
            break;
        }
    }

    printf("quit rtc test\n");
}

void FactoryTest::_rtc_wakeup_test()
{
    _canvas->fillScreen(TFT_BLACK);
    _canvas->setCursor(0, 10);
    _canvas->setFont(&fonts::efontCN_24);
    _canvas->setTextColor(TFT_YELLOW);
    _canvas->setTextSize(1);
    _canvas->printf(" [RTC Wakeup Test]\n - Remove power -\n\n Press button [Power Off]\n Will auto boot in seconds");
    _canvas_update();

    _wait_next();

    _rtc.clearIRQ();
    _rtc.SetAlarmIRQ(4);
    _canvas->fillScreen(TFT_BLACK);
    _canvas_update();
    delay(500);

    digitalWrite(POWER_HOLD_PIN, 0);

    while (1)
    {
        printf("%d\n", _btn_pwr.read());
        delay(50);
    }
}

void FactoryTest::_rtc_wakeup_test_user()
{
    printf("rtc wakeup test\n");
    _canvas->setFont(&fonts::Font0);
    _canvas->fillScreen((uint32_t)0xC6D5EF);

    _enc.setPosition(5);
    _enc_pos = 5;

    // Choose wake up time
    char string_buffer[24];
    while (1)
    {
        _canvas->fillScreen((uint32_t)0xC6D5EF);

        _canvas->fillRect(0, 0, 240, 25, (uint32_t)0x46556F);
        _canvas->setTextSize(2);
        _canvas->setTextColor((uint32_t)0xC6D5EF);
        snprintf(string_buffer, 20, "Sleep & Wake Up");
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 5);

        _canvas->setTextColor((uint32_t)0x46556F);
        _canvas->setCursor(0, 30);
        _canvas->printf(" Press Button Sleep\n Wake Up In:");

        _canvas->setTextSize(5);
        _canvas->setTextColor((uint32_t)0x46556F);
        snprintf(string_buffer, 20, "%ds", _enc_pos);
        _canvas->drawCenterString(string_buffer, _canvas->width() / 2, 85);

        _canvas_update();

        _check_encoder();

        // Limit
        if (_enc.getPosition() > 20)
            _enc.setPosition(20);
        if (_enc.getPosition() < 5)
            _enc.setPosition(5);

        if (_check_next())
            break;
    }

    // Go
    printf("sleep %ds\n", _enc.getPosition());
    _rtc.clearIRQ();
    _rtc.SetAlarmIRQ(_enc.getPosition());
    _canvas->fillScreen(TFT_BLACK);
    _canvas_update();
    delay(500);

    digitalWrite(POWER_HOLD_PIN, 0);

    while (1)
    {
        printf("%d\n", _btn_pwr.read());
        delay(50);
    }
}
