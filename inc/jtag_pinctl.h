/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef JTAG_PINCTL_H_
#define JTAG_PINCTL_H_

#include <stdint.h>

struct jtagPinLocation{
    unsigned char ucPin;
    unsigned int uiGPIOPort;
    unsigned char ucGPIOPin;
};

enum jtagPinBit{
    JTAG_TDI = 1,
    JTAG_TMS = 2,
    JTAG_RST = 4,
    JTAG_TCK = 8
};

int jtag_pinctl_init(void);
int jtag_pinctl_doClock(uint8_t active_pins);

#endif
