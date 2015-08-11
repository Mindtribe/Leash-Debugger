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

#include "cc3200_icepick.h"
#include "cc3200_jtagdp.h"
#include "jtag_scan.h"
#include "common.h"
#include "error.h"

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

    //ICEPICK router detection and configuration
    if(cc3200_icepick_init() == RET_FAILURE) error_wait(ERROR_UNKNOWN);
    if(cc3200_icepick_detect() == RET_FAILURE) error_wait(ERROR_UNKNOWN);
    if(cc3200_icepick_connect() == RET_FAILURE) error_wait(ERROR_UNKNOWN);
    if(cc3200_icepick_configure() == RET_FAILURE) error_wait(ERROR_UNKNOWN);

    //ARM core debug interface (JTAG-DP) detection
    if(cc3200_jtagdp_init(6, ICEPICK_IR_BYPASS, 1, 1) == RET_FAILURE) error_wait(ERROR_UNKNOWN);
    if(cc3200_jtagdp_detect() == RET_FAILURE) error_wait(ERROR_UNKNOWN);

    //Try reading, then writing, then reading a DP register.
    uint32_t result1, result2;
    if(cc3200_jtagdp_DPACC_read(0x08,&result1) == RET_FAILURE) error_wait(ERROR_UNKNOWN);
    if(cc3200_jtagdp_DPACC_write(0x08,0xF0) == RET_FAILURE) error_wait(ERROR_UNKNOWN);
    if(cc3200_jtagdp_DPACC_read(0x08,&result2) == RET_FAILURE) error_wait(ERROR_UNKNOWN);

    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);

    while(1){};

    return RET_SUCCESS;
}
