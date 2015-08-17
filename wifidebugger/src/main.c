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
    ClearTerm();
    PinMuxConfig();

    mem_log_add("Init", 0);

    //(error) logging in memory init
    clear_errors();
    mem_log_clear();
    mem_log_add("Start of main().", 0);

    GPIO_IF_LedConfigure(LED1|LED2|LED3);
    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);

    //hard reset
    jtag_scan_init();
    jtag_scan_hardRst();

    //ICEPICK router detection and configuration
    if(cc3200_icepick_init() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    if(cc3200_icepick_detect() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    if(cc3200_icepick_connect() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    if(cc3200_icepick_configure() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    mem_log_add("Detected and configured ICEPICK router.", 0);

    //ARM core debug interface (JTAG-DP) detection
    if(cc3200_jtagdp_init(6, ICEPICK_IR_BYPASS, 1, 1) == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    if(cc3200_jtagdp_detect() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    mem_log_add("Detected core's JTAG-DP port.", 0);

    //Clear control/status register.
    if(cc3200_jtagdp_clearCSR() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    mem_log_add("Cleared JTAG-DP CSR.", 0);

    //Read control/status register.
    uint32_t csr;
    //if(cc3200_jtagdp_checkCSR(&csr) == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    //mem_log_add("CSR value:", csr);

    //powerup
    if(cc3200_jtagdp_powerUpDebug() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    mem_log_add("Powered up SoC and debug logic.", 0);

    cc3200_jtagdp_checkCSR(&csr);
    mem_log_add("CSR value:", csr);

    if(cc3200_jtagdp_readAPs() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    mem_log_add("Read out all 16 AP's IDCODES connected to JTAG-DP.", 0);

    //core module
    if(cc3200_core_init() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_detect() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    mem_log_add("Initialized and detected MEM-AP of cortex M4.", 0);

    //enable debug
    if(cc3200_core_debug_enable() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);

    //turn the orange LED off on target (which has "testapp" running)
    uint32_t ledaddr;
    if(cc3200_core_read_mem_addr(0x0000000020005270, &ledaddr) == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_write_mem_addr(0x0000000020005270, 0) == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);

    cc3200_jtagdp_checkCSR(&csr);
    mem_log_add("CSR value:", csr);

    //halt the core
    if(cc3200_core_debug_halt() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    mem_log_add("Entered debug and halted core.", 0);

    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);

    while(1){};

    return RET_SUCCESS;
}
