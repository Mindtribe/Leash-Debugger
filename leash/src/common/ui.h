/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef UI_H_
#define UI_H_

#include "hw_memmap.h"

enum user_switches{
    SWITCH_APSEL = 0,
    NUM_SWITCHES
};

//these constants represent LED blinking patterns.
//The individual bits of the constant will be shifted one place every 100ms.
//Whenever the LSB is non-zero the LED will be on, otherwise it will be off.
#define LED_BLINK_PATTERN_ERROR 0x33333333
#define LED_BLINK_PATTERN_WIFI_SCANNING 0xF0F0F0F0
#define LED_BLINK_PATTERN_WIFI_FAILED 0x33333333
#define LED_BLINK_PATTERN_WIFI_CONNECTED 0xFFFFFFFF
#define LED_BLINK_PATTERN_WIFI_CONNECTING 0x30303030
#define LED_BLINK_PATTERN_WIFI_AP 0x3FFF3FFF
#define LED_BLINK_PATTERN_JTAG_INIT 0xF0F0F0F0
#define LED_BLINK_PATTERN_JTAG_HALTED 0xFFFFFFFF
#define LED_BLINK_PATTERN_JTAG_RUNNING 0xF000F000
#define LED_BLINK_PATTERN_JTAG_FAILED 0x33333333

//pin configurations assertion
#ifndef BOARD_LAUNCHPAD
#ifndef BOARD_RBL_WIFIMINI
#error No board type was specified: please add the appropriate preprocessor define to your compile command (BOARD_LAUNCHPAD or BOARD_RBL_WIFIMINI)
#endif //BOARD_RBL_WIFIMINI
#endif //BOARD_LAUNCHPAD

#endif
