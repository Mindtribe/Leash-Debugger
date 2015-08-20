/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "cc3200.h"
#include "common.h"
#include "error.h"
#include "mem_log.h"
#include "jtag_scan.h"
#include "cc3200_icepick.h"
#include "cc3200_core.h"
#include "cc3200_jtagdp.h"

struct target_al_interface cc3200_interface = {
    .target_init = &cc3200_init,
    .target_reset = &cc3200_reset,
    .target_halt = &cc3200_halt,
    .target_continue = &cc3200_continue,
    .target_mem_read = &cc3200_mem_read,
    .target_mem_write = &cc3200_mem_write,
    .target_get_gdb_reg_string = &cc3200_get_gdb_reg_string,
    .target_put_gdb_reg_string = &cc3200_put_gdb_reg_string
};

struct cc3200_state_t{
    char regstring[CC3200_REG_LAST+1];
};
struct cc3200_state_t cc3200_state = {
    .regstring = {0}
};

static void cc3200_byteToHex(uint8_t byte, char* dst)
{
    wfd_byteToHex(byte, dst);
}

static uint8_t cc3200_hexToByte(char* hex){
    return wfd_hexToByte(hex);
}

int cc3200_init(void)
{
    //hard reset
    jtag_scan_init();
    jtag_scan_hardRst();

    //ICEPICK router detection and configuration
    if(cc3200_icepick_init() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_icepick_detect() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_icepick_connect() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_icepick_configure() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    mem_log_add("CC3200 - ICEPICK OK.", 0);

    //ARM core debug interface (JTAG-DP) detection
    if(cc3200_jtagdp_init(6, ICEPICK_IR_BYPASS, 1, 1) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_jtagdp_detect() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_jtagdp_clearCSR() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    uint32_t csr;
    if(cc3200_jtagdp_checkCSR(&csr) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    mem_log_add("CC3200 - JTAG-DP OK.", 0);

    //powerup
    if(cc3200_jtagdp_powerUpDebug() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    mem_log_add("CC3200 - Powerup OK.", 0);

    if(cc3200_jtagdp_readAPs() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    mem_log_add("CC3200 - IDCODEs OK.", 0);

    //core module
    if(cc3200_core_init() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_detect() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    mem_log_add("CC3200 - MEM-AP OK.", 0);

    //enable debug
    if(cc3200_core_debug_enable() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    mem_log_add("CC3200 - Debug Enabled.", 0);

    return RET_SUCCESS;
}

int cc3200_reset(void)
{
    //TODO: do a proper "warm" reset. This takes ages.

    return cc3200_init();
}

int cc3200_halt(void)
{
    //halt the core
    if(cc3200_core_debug_halt() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);

    return RET_SUCCESS;
}

int cc3200_continue(void)
{
    return RET_SUCCESS;
}

int cc3200_mem_read(uint32_t addr, uint32_t* dst)
{
    return cc3200_core_read_mem_addr(addr, dst);
}

int cc3200_mem_write(uint32_t addr, uint32_t value)
{
    return cc3200_core_write_mem_addr(addr, value);
}

int cc3200_get_gdb_reg_string(char** string)
{
    struct cc3200_reg_list reglst;

    for(enum cc3200_reg_index i=0; i<CC3200_REG_LAST; i++){
        if(cc3200_core_read_reg(i, &(((uint32_t *)&reglst)[i])) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    }

    cc3200_state.regstring[8*CC3200_REG_LAST] = 0; //zero-terminate

    for(enum cc3200_reg_index i=0; i<CC3200_REG_LAST; i++){
        //bytewise convert the register to hex characters.
        cc3200_byteToHex((((uint32_t*)&reglst)[i]>>0) & 0xFF, &(cc3200_state.regstring[8*i+0]));
        cc3200_byteToHex((((uint32_t*)&reglst)[i]>>8) & 0xFF, &(cc3200_state.regstring[8*i+2]));
        cc3200_byteToHex((((uint32_t*)&reglst)[i]>>16) & 0xFF, &(cc3200_state.regstring[8*i+4]));
        cc3200_byteToHex((((uint32_t*)&reglst)[i]>>24) & 0xFF, &(cc3200_state.regstring[8*i+6]));
    }

    *string = cc3200_state.regstring; //point to the string

    return RET_SUCCESS;
}

int cc3200_put_gdb_reg_string(char* string)
{
    //TODO: string validity tests

    for(int i = 0; string[i] != 0 && i<CC3200_REG_LAST; i++){
        if(string[i*8] == 'x') continue; //skip this register

        uint32_t reg = 0; //the register value
        reg |= (cc3200_hexToByte(&(string[i*8+0])) << 0);
        reg |= (cc3200_hexToByte(&(string[i*8+2])) << 8);
        reg |= (cc3200_hexToByte(&(string[i*8+4])) << 16);
        reg |= (cc3200_hexToByte(&(string[i*8+6])) << 24);

        if(cc3200_core_write_reg(i, reg) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    }
    return RET_SUCCESS;
}

