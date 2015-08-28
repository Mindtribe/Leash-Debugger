/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */


#include <stdio.h>

#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"

#include "gpio_if.h"
#include "pin_mux_config.h"

volatile int doLED = 1;

static int BoardInit(void);

static int BoardInit(void)
{
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);
    PRCMCC3200MCUInit();

    return 1;
}

void semihost_printstr(char* msg)
{
    //set the arguments
    asm volatile("mov R0,#0x04");
    asm volatile("mov R1,%[message]" : : [message]"r" (msg));
    asm volatile("BKPT 0xAB");

    return;
}

int main(void)
{
    BoardInit();

    PinMuxConfig();
    GPIO_IF_LedConfigure(LED1|LED2|LED3);
    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);

    //Semihosting call to say hello
    semihost_printstr("\n\nTestApp: I am alive! :)\nNow, let me echo you.\n>");

    unsigned int j = 0;
    while(1){
        for(int i=0; i<40000; i++){};
        if(doLED) GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
        else GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);

        j++;
        if(j%100 == 0) GPIO_IF_LedToggle(MCU_RED_LED_GPIO);

    };

    return 1;
}
