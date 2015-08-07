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

#include "jtag_scan.h"
#include "common.h"

static int BoardInit(void);

static int BoardInit(void)
{
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();

    return RET_SUCCESS;
}

int main(void)
{
    BoardInit();

    PinMuxConfig();
    GPIO_IF_LedConfigure(LED1|LED2|LED3);
    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    jtag_scan_init();

    jtag_scan_hardRst();
    jtag_scan_rstStateMachine();
    jtag_scan_shiftIR(0xFF00F0F0, 32);
    jtag_scan_shiftIR(0xFF00F0F0, 32);
    while(1)
    {
        jtag_scan_shiftIR(jtag_scan_getShiftOut(), 32);
        MAP_UtilsDelay(400000);
        GPIO_IF_LedToggle(MCU_RED_LED_GPIO);
    }

    /* Old "Unit Tests":


    while(1){
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        for(int i=0; i<100; i++){
            jtag_pinctl_doClock(JTAG_TMS | JTAG_RST);
            jtag_pinctl_doClock(JTAG_TDI);
        }
        GPIO_IF_LedOff(MCU_RED_LED_GPIO);
        for(int i=0; i<100; i++){
            jtag_pinctl_doClock(JTAG_TMS | JTAG_RST);
            jtag_pinctl_doClock(JTAG_TDI);
        }
    }

    while(1){
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
        GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
        MAP_UtilsDelay(800000);
        GPIO_IF_LedOff(MCU_RED_LED_GPIO);
        GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
        GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
        MAP_UtilsDelay(800000);
    }

     */

    return RET_SUCCESS;
}
