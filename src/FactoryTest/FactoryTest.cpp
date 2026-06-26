/**
 * @file FactoryTest.cpp
 * @brief Hardware initialization and factory test entry
 */
#include "FactoryTest.h"

void FactoryTest::init()
{
    _power_on();

    _rtc_init();

    _key_init();

    _disp_init();

    // Encoder init
    _enc.attachHalfQuad(40, 41);
    _enc.setCount(0);

    // if (_check_test_mode())
    // {
    //     start_factory_test();
    // }
}

void FactoryTest::start_factory_test()
{
    _wifi_start_test_task();
    _disp_test();
    _rtc_test();
    _encoder_test_new();
    _io_test();
    _wifi_test();
    _ble_test();
    _rtc_wakeup_test();
}
