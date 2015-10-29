/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef LED_H_
#define LED_H_

enum LED_ID{
    LED_1 = 0,
    LED_3,
    LED_2,
    NUM_LEDS
};

void SetLED(unsigned int id, unsigned int value);
void ClearLED(unsigned int id);
void InitLED(void);
void SetLEDBlink(unsigned int id, unsigned int pattern);

#endif
