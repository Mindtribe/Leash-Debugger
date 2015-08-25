/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "rom_map.h"
#include "utils.h"

#include "misc_hal.h"

void delay_loop(int iterations)
{
    MAP_UtilsDelay(iterations);
    return;
}
