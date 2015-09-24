/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef SWITCH_H_
#define SWITCH_H_

enum user_switches{
    SWITCH_2 = 0,
    SWITCH_3,
    NUM_SWITCHES
};

#define AP_SWITCH SWITCH_3 //switch to choose AP mode on startup

unsigned int GetUserSwitch(unsigned int id);

#endif
