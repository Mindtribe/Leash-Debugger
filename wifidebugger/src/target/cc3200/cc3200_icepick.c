/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include <stdint.h>

#include "cc3200_icepick.h"
#include "jtag_scan.h"
#include "common.h"
#include "misc_hal.h"
#include "error.h"

//state struct
struct cc3200_icepick_state_t{
    unsigned char initialized;
    unsigned char detected;
    unsigned char connected;
    unsigned char configured;
    struct cc3200_icepick_properties_t properties;
};
//instantiate state struct
struct cc3200_icepick_state_t cc3200_icepick_state = {
    .initialized = 0,
    .detected = 0,
    .connected = 0,
    .configured = 0,
    .properties = {
        .IDCODE_MANUFACTURER = 0,
        .IDCODE_PARTNUMBER = 0,
        .IDCODE_VERSION = 0,
        .ICEPICKCODE_MAJORVERSION = 0,
        .ICEPICKCODE_MINORVERSION = 0,
        .ICEPICKCODE_TESTTAPS = 0,
        .ICEPICKCODE_EMUTAPS = 0,
        .ICEPICKCODE_ICEPICKTYPE = 0,
        .ICEPICKCODE_CAPABILITIES = 0
    }
};

int cc3200_icepick_init(void)
{
    if(cc3200_icepick_state.initialized) return RET_SUCCESS;

    if(jtag_scan_init() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    cc3200_icepick_state.initialized = 1;
    return RET_SUCCESS;
}

int cc3200_icepick_detect(void)
{
    if(!cc3200_icepick_state.initialized) RETURN_ERROR(ERROR_UNKNOWN);

    jtag_scan_rstStateMachine();

    jtag_scan_shiftIR(ICEPICK_IR_IDCODE, ICEPICK_IR_LEN, JTAG_STATE_SCAN_PAUSE);
    jtag_scan_shiftDR(0,32, JTAG_STATE_SCAN_RUNIDLE); //get 32-bit IDCODE
    uint32_t idcode = jtag_scan_getShiftOut();

    cc3200_icepick_state.properties.IDCODE_MANUFACTURER = (uint16_t) ((idcode>>1) & 0x7FF);
    cc3200_icepick_state.properties.IDCODE_PARTNUMBER = (uint16_t) ((idcode>>12) & 0xFFFF);
    cc3200_icepick_state.properties.IDCODE_VERSION = (uint8_t) (idcode>>28);

    //check for validity
    if( (cc3200_icepick_state.properties.IDCODE_MANUFACTURER != IDCODE_MANUFACTURER_TI) ||
            (cc3200_icepick_state.properties.IDCODE_PARTNUMBER != IDCODE_PARTNUMBER_CC3200) ){
        cc3200_icepick_state.detected = 0;
        RETURN_ERROR(ERROR_UNKNOWN);
    }

    jtag_scan_shiftIR(ICEPICK_IR_ICEPICKCODE, ICEPICK_IR_LEN, JTAG_STATE_SCAN_PAUSE);
    jtag_scan_shiftDR(0,32, JTAG_STATE_SCAN_RUNIDLE); //get 32-bit ICEPICKCODE
    uint32_t icepickcode = jtag_scan_getShiftOut();

    cc3200_icepick_state.properties.ICEPICKCODE_CAPABILITIES = (uint8_t) ((icepickcode) & 0xF);
    cc3200_icepick_state.properties.ICEPICKCODE_ICEPICKTYPE = (uint16_t) ((icepickcode>>4) & 0xFFF);
    cc3200_icepick_state.properties.ICEPICKCODE_EMUTAPS = (uint8_t) ((icepickcode>>16) & 0xF);
    cc3200_icepick_state.properties.ICEPICKCODE_TESTTAPS = (uint8_t) ((icepickcode>>20) & 0xF);
    cc3200_icepick_state.properties.ICEPICKCODE_MINORVERSION = (uint8_t) ((icepickcode>>24) & 0xF);
    cc3200_icepick_state.properties.ICEPICKCODE_MAJORVERSION = (uint8_t) ((icepickcode>>28));

    //check for validity
    if(cc3200_icepick_state.properties.ICEPICKCODE_ICEPICKTYPE != ICEPICKCODE_TYPE_C){
        cc3200_icepick_state.detected = 0;
        RETURN_ERROR(ERROR_UNKNOWN);
    }

    cc3200_icepick_state.detected = 1;
    return RET_SUCCESS;
}

int cc3200_icepick_connect(void)
{
    if(!cc3200_icepick_state.initialized || !cc3200_icepick_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    //note: the reason for using these specific ICEPICK commands can be found in TI's ICEPICK type C technical reference manual.

    if(jtag_scan_shiftIR(ICEPICK_IR_CONNECT, ICEPICK_IR_LEN, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(jtag_scan_shiftDR(0b10001001, 8, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    cc3200_icepick_state.connected = 1;
    return RET_SUCCESS;
}

int cc3200_icepick_disconnect(void)
{
    if(!cc3200_icepick_state.initialized || !cc3200_icepick_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    //note: the reason for using these specific ICEPICK commands can be found in TI's ICEPICK type C technical reference manual.

    if(jtag_scan_shiftIR(ICEPICK_IR_CONNECT, ICEPICK_IR_LEN, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(jtag_scan_shiftDR(0b10000110, 8, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    cc3200_icepick_state.connected = 0;
    return RET_SUCCESS;
}

int cc3200_icepick_router_command(uint8_t rw, uint8_t block, uint8_t reg, uint32_t value, enum jtag_state_scan toState)
{
    if(!cc3200_icepick_state.initialized || !cc3200_icepick_state.detected || !cc3200_icepick_state.connected) RETURN_ERROR(ERROR_UNKNOWN);

    uint32_t data_reg = 0;
    data_reg |= rw ? (1<<31) : 0; //read/write bit
    data_reg |= ((block&0x07) << 28);
    data_reg |= ((reg&0x0F) << 24);
    data_reg |= (value&0xFFFFFF);

    if(jtag_scan_shiftIR(ICEPICK_IR_ROUTER, ICEPICK_IR_LEN, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(jtag_scan_shiftDR(data_reg, 32, toState) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    return RET_SUCCESS;
}

int cc3200_icepick_configure(void)
{
    if(!cc3200_icepick_state.initialized || !cc3200_icepick_state.detected || !cc3200_icepick_state.connected) RETURN_ERROR(ERROR_UNKNOWN);

    //note: this sequence of commands has been derived from OpenOCD's config file for icepick type c (icepick.cfg).
    //another source for them was not (yet) found.

    //reason for command unclear
    if(cc3200_icepick_router_command(1, 0, 1, 0x00100, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    //enable power/clock of TAP
    if(cc3200_icepick_router_command(1, 2, 0, 0x100048, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    //enable debug default mode
    if(cc3200_icepick_router_command(1, 2, 0, 0x102048, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    //select TAP
    if(cc3200_icepick_router_command(1, 2, 0, 0x102148, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    //set ICEPICK TAP to BYPASS
    if(jtag_scan_shiftIR(ICEPICK_IR_BYPASS, ICEPICK_IR_LEN, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    //wait for a while
    delay_loop(100000);

    //TODO: this shouldn't be necessary, but it has been observed that the IR only starts working again after a double dummy write for some reason.
    //Figure out why!
    if(jtag_scan_shiftIR(0b1111111111, 10, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(jtag_scan_shiftIR(0b1111111111, 10, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    cc3200_icepick_state.configured = 1;
    return RET_SUCCESS;
}
