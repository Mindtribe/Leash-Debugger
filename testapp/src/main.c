/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */


#include <stdio.h>
#include <stdint.h>

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

struct semihostArg{
    uint32_t word1;
    uint32_t word2;
    uint32_t word3;
    uint32_t word4;
};

static int BoardInit(void);
void semihost_printstr(char* msg);
uint32_t semihost_getinput(char* buf, uint32_t len);

int main(void)
{
    BoardInit();

    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);

    semihost_printstr("\n\nTestApp: I am alive! :)\nNow, let me echo you.\n>");

    char buffer[256];

    for(;;){
        GPIO_IF_LedToggle(MCU_RED_LED_GPIO);

        semihost_getinput(buffer, 256);
        semihost_printstr(buffer);
        semihost_printstr("\n>");
    }

    return 1;
}

static int BoardInit(void)
{
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);
    PRCMCC3200MCUInit();

    PinMuxConfig();
    GPIO_IF_LedConfigure(LED1|LED2|LED3);
    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    return 1;
}

void semihost_printstr(char* msg)
{
    //set the arguments
    asm volatile("mov R0,#0x04");
    asm volatile("mov R1,%[message]" : : [message]"r" (msg));

    //break and let debugger do its magic
    asm volatile("BKPT 0xAB");

    //TODO: if wanting to error-check, read R0 here

    return;
}

uint32_t semihost_getinput(char* buf, uint32_t len)
{
    struct semihostArg arg;
    arg.word1 = 0;
    arg.word2 = (uint32_t)buf;
    arg.word3 = len;

    uint32_t response = 0;

    //set the arguments
    asm volatile("mov R0, #0x06");
    asm volatile("mov R1, %[arguments]" : : [arguments]"r" (&arg));

    //break and let the debugger do its magic
    asm volatile("BKPT 0xAB");

    //read the response
    asm volatile("mov %[result], R0" : [result]"=r" (response));

    return response;
}
