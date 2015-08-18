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
#include "uart_if.h"
#include "pin_mux_config.h"

#include "cc3200_icepick.h"
#include "cc3200_jtagdp.h"
#include "cc3200_core.h"
#include "jtag_scan.h"
#include "common.h"
#include "error.h"
#include "mem_log.h"
#include "misc_hal.h"
#include "gdb_helpers.h"
#include "target_al.h"
#include "cc3200.h"
#include "gdbserver.h"

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

    //UART terminal
    InitTerm();
    PinMuxConfig();

    mem_log_add("Init", 0);

    //(error) logging in memory init
    clear_errors();
    mem_log_clear();
    mem_log_add("Start of main().", 0);

    GPIO_IF_LedConfigure(LED1|LED2|LED3);
    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);

    gdbserver_init((void*) &TermPutChar, (void*) &TermGetChar);
    gdbserver_loop();

    //Some tests.
    struct target_al_interface *target_if = &cc3200_interface;

    if((*target_if->target_init)() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    //turn LED off:
    if((*target_if->target_mem_write)(0x0000000020005270, 0) == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    if((*target_if->target_halt)() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);

    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);

    while(1){};

    return RET_SUCCESS;
}
