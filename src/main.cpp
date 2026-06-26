/**
 * @file main.cpp
 * @brief Entry point for M5DinMeter Dock controller
 */
#include "FactoryTest/FactoryTest.h"

static FactoryTest ft;

void view_create(FactoryTest* ft);
void view_update();

void setup()
{
    ft.init();
    view_create(&ft);
}

void loop()
{
    view_update();
}
