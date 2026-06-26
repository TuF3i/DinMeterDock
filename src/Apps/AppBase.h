/**
 * @file AppBase.h
 * @brief Abstract base class for all user-facing applications
 */
#pragma once
#include "../Hardware/Hardware.h"

class AppBase
{
public:
    virtual ~AppBase() = default;
    virtual void run(Hardware& hw) = 0;
};
