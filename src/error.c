/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "gpio_if.h"

#include "error.h"

void error_wait(int error_code)
{
    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    while(1){};
    return;
}
