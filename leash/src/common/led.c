/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "led.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "timer.h"
#include "timer_if.h"

#include "gpio_al.h"
#include "ui.h"

struct led_state_t{
    unsigned int blink_enabled;
    unsigned int blink_pattern;
    unsigned int blink_bit;
};

static struct led_state_t led_state[NUM_LEDS] = {{0}};

#ifdef BOARD_LAUNCHPAD
static const char ledGPIOs[NUM_LEDS] = {
    11, //Green
    10, //Orange
    9 //Red
};
#endif
#ifdef BOARD_RBL_WIFIMINI
static const char ledGPIOs[NUM_LEDS] = {
    30, //Orange
    0,
    0
};
#endif

static void Int_LEDBlink(void);

void SetLED(unsigned int id, unsigned int value){
    if(id>=NUM_LEDS) return;
    GPIO_SET_LED(ledGPIOs[id], value);
}
void ClearLED(unsigned int id){
    if(id>=NUM_LEDS) return;
    led_state[id].blink_enabled = 0;
    GPIO_SET_LED(ledGPIOs[id], 0);
}

void InitLED(void)
{
    GPIO_SET_PIN(ledGPIOs[LED_1], 0);
    GPIO_SET_PIN(ledGPIOs[LED_3], 0);
    GPIO_SET_PIN(ledGPIOs[LED_2], 0);

    for(int i=0; i<NUM_LEDS; i++) {led_state[i].blink_bit = 1;}

    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC, TIMER_A, 0);
    Timer_IF_IntSetup(TIMERA0_BASE, TIMER_A, Int_LEDBlink);
    Timer_IF_Start(TIMERA0_BASE, TIMER_A, 100);
}

void SetLEDBlink(unsigned int id, unsigned int pattern)
{
    if(id>=NUM_LEDS) return;
    led_state[id].blink_pattern = pattern;
    led_state[id].blink_enabled = 1;
}

static void Int_LEDBlink(void)
{
    for(int i=0; i<NUM_LEDS; i++){
        if(led_state[i].blink_enabled){
            led_state[i].blink_bit = (led_state[i].blink_bit<<1)|(led_state[i].blink_bit>>31); //rotate the bit
            SetLED(i,(led_state[i].blink_pattern & led_state[i].blink_bit));
        }
    }
    Timer_IF_InterruptClear(TIMERA0_BASE);
}
