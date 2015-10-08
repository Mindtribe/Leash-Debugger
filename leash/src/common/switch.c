/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "switch.h"
#include "gpio_if.h"
#include "ui.h"

const unsigned char switch_pins[NUM_SWITCHES] = {
    22,
    13
};

unsigned int GetUserSwitch(unsigned int id)
{
    if(id>=NUM_SWITCHES) return 0;
    return GPIO_IF_SwStatus(switch_pins[id]);
}
