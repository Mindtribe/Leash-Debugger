/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "switch.h"
#include "ui.h"
#include "gpio_al.h"

#ifdef BOARD_LAUNCHPAD
const unsigned char switch_GPIOs[NUM_SWITCHES] = {
    13
};
#endif
#ifdef BOARD_RBL_WIFIMINI
const unsigned char switch_GPIOs[NUM_SWITCHES] = {
    22
};
#endif

unsigned int GetUserSwitch(unsigned int id)
{
    if(id>=NUM_SWITCHES) return 0;
    return GPIO_GET_PIN(switch_GPIOs[id]);
}
