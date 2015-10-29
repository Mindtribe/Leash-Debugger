/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef GPIO_AL_
#define GPIO_AL_

#include "hw_memmap.h"
#include "hw_types.h"
#include "ui.h"

extern const unsigned long gpioRegs[];

#ifdef BOARD_RBL_WIFIMINI
#define LED_ACTIVE_LOW //if defined, this sets LEDs to "active low".
#endif

#define GPIO_SET_PIN(GPIONUM, ONOFF) (HWREG((gpioRegs[(((int)GPIONUM)/8)]+(1<<((GPIONUM%8)+2))))=((ONOFF)*((1<<((GPIONUM%8))))))
#define GPIO_GET_PIN(GPIONUM) (HWREG((gpioRegs[(((int)GPIONUM)/8)]+(1<<((GPIONUM%8)+2)))))

#ifdef LED_ACTIVE_LOW
#define GPIO_SET_LED(GPIONUM, ONOFF) (GPIO_SET_PIN(GPIONUM, ((ONOFF)==0)))
#else
#define GPIO_SET_LED(GPIONUM, ONOFF) (GPIO_SET_PIN(GPIONUM, ((ONOFF)!=0)))
#endif

#endif
