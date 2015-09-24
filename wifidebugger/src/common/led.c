/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "led.h"
#include "gpio_if.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "timer.h"
#include "timer_if.h"

struct led_state_t{
    unsigned int blink_enabled;
    unsigned int blink_pattern;
    unsigned int blink_bit;
};

struct led_state_t led_state[NUM_LEDS] = {{0}};

char lednums[NUM_LEDS];

void Int_LEDBlink(void);

void SetLED(unsigned int id, unsigned int value){
    if(value){GPIO_IF_LedOn(lednums[id]);}
    else{GPIO_IF_LedOff(lednums[id]);}
}
void ClearLED(unsigned int id){
    led_state[id].blink_enabled = 0;
    GPIO_IF_LedOff(lednums[id]);
}

void InitLED(void)
{
    GPIO_IF_LedConfigure(LED1|LED2|LED3);
    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    lednums[LED_GREEN] = MCU_GREEN_LED_GPIO;
    lednums[LED_ORANGE] = MCU_ORANGE_LED_GPIO;
    lednums[LED_RED] = MCU_RED_LED_GPIO;

    for(int i=0; i<NUM_LEDS; i++) {led_state[i].blink_bit = 1;}

    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC, TIMER_A, 0);
    Timer_IF_IntSetup(TIMERA0_BASE, TIMER_A, Int_LEDBlink);
    Timer_IF_Start(TIMERA0_BASE, TIMER_A, 100);
}

void SetLEDBlink(unsigned int id, unsigned int pattern)
{
    led_state[id].blink_pattern = pattern;
    led_state[id].blink_enabled = 1;
}

void Int_LEDBlink(void)
{
    for(int i=0; i<NUM_LEDS; i++){
        if(led_state[i].blink_enabled){
            led_state[i].blink_bit = (led_state[i].blink_bit<<1)|(led_state[i].blink_bit>>31); //rotate the bit
            SetLED(i,(led_state[i].blink_pattern & led_state[i].blink_bit));
        }
    }
    Timer_IF_InterruptClear(TIMERA0_BASE);
}
