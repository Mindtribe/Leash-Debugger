/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "cc3200_icepick.h"

#include <stdint.h>

#include "jtag_scan.h"
#include "misc_hal.h"
#include "error.h"

#define IDCODE_MANUFACTURER_TI 0x17
//note: this part number was read from the device, and online resources suggest
//that this is indeed the part number of a CC3200. However, no official documentation
//has been found that gives any clue whether CC3200's might exist with other
//part numbers. To be determined.
#define IDCODE_PARTNUMBER_CC3200 0xB97C

#define ICEPICKCODE_TYPE_C 0x1CC

#define CC3200_ICEPICK_SCR_FREERUNNINGTCK (1<<12)
#define CC3200_ICEPICK_SCR_WARMRESET (1)

#define CC3200_ICEPICK_SCR_BLOCK 0
#define CC3200_ICEPICK_SCR_REG 1

#define CC3200_ICEPICK_SDTAP0_BLOCK 2
#define CC3200_ICEPICK_SDTAP0_REG 0

#define CC3200_ICEPICK_SDTAP_FORCEACTIVE (1<<3)
#define CC3200_ICEPICK_SDTAP_FORCEPOWER (1<<6)
#define CC3200_ICEPICK_SDTAP_INHIBITSLEEP (1<<20)
#define CC3200_ICEPICK_SDTAP_DEBUGCONNECT (1<<13)
#define CC3200_ICEPICK_SDTAP_TAPSELECT (1<<8)

struct cc3200_icepick_properties_t{
    uint16_t IDCODE_MANUFACTURER;
    uint16_t IDCODE_PARTNUMBER;
    uint8_t IDCODE_VERSION;
    uint8_t ICEPICKCODE_MAJORVERSION;
    uint8_t ICEPICKCODE_MINORVERSION;
    uint8_t ICEPICKCODE_TESTTAPS;
    uint8_t ICEPICKCODE_EMUTAPS;
    uint16_t ICEPICKCODE_ICEPICKTYPE;
    uint8_t ICEPICKCODE_CAPABILITIES;
};

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

int cc3200_icepick_deinit(void)
{
    cc3200_icepick_state.initialized = 0;

    return RET_SUCCESS;
}

int cc3200_icepick_init(void)
{
    if(cc3200_icepick_state.initialized) {return RET_SUCCESS;}

    if(jtag_scan_init() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    cc3200_icepick_state.initialized = 1;
    return RET_SUCCESS;
}

int cc3200_icepick_detect(void)
{
    if(!cc3200_icepick_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);}

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
        {RETURN_ERROR(ERROR_UNKNOWN);}
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
        {RETURN_ERROR(ERROR_UNKNOWN);}
    }

    cc3200_icepick_state.detected = 1;
    return RET_SUCCESS;
}

int cc3200_icepick_connect(void)
{
    if(!cc3200_icepick_state.initialized || !cc3200_icepick_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}

    //note: the reason for using these specific ICEPICK commands can be found in TI's ICEPICK type C technical reference manual.

    if(jtag_scan_shiftIR(ICEPICK_IR_CONNECT, ICEPICK_IR_LEN, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(jtag_scan_shiftDR(0x89, 8, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    cc3200_icepick_state.connected = 1;
    return RET_SUCCESS;
}

int cc3200_icepick_disconnect(void)
{
    if(!cc3200_icepick_state.initialized || !cc3200_icepick_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}

    //note: the reason for using these specific ICEPICK commands can be found in TI's ICEPICK type C technical reference manual.

    if(jtag_scan_shiftIR(ICEPICK_IR_CONNECT, ICEPICK_IR_LEN, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(jtag_scan_shiftDR(0x86, 8, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    cc3200_icepick_state.connected = 0;
    return RET_SUCCESS;
}

int cc3200_icepick_router_command(uint8_t rw, uint8_t block, uint8_t reg, uint32_t value, enum jtag_state_scan toState)
{
    if(!cc3200_icepick_state.initialized || !cc3200_icepick_state.detected || !cc3200_icepick_state.connected) {RETURN_ERROR(ERROR_UNKNOWN);}

    uint32_t data_reg = 0;
    data_reg |= rw ? (1<<31) : 0; //read/write bit
    data_reg |= ((block&0x07) << 28);
    data_reg |= ((reg&0x0F) << 24);
    data_reg |= (value&0xFFFFFF);

    if(jtag_scan_shiftIR(ICEPICK_IR_ROUTER, ICEPICK_IR_LEN, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(jtag_scan_shiftDR(data_reg, 32, toState) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}

int cc3200_icepick_configure(void)
{
    if(!cc3200_icepick_state.initialized || !cc3200_icepick_state.detected || !cc3200_icepick_state.connected) {RETURN_ERROR(ERROR_UNKNOWN);}

    //note: this sequence of commands has been derived from OpenOCD's config file for icepick type c (icepick.cfg).
    //another source for them was not (yet) found.

    //Set free-running TCK mode
    if(cc3200_icepick_router_command(1, CC3200_ICEPICK_SCR_BLOCK, CC3200_ICEPICK_SCR_REG,
            CC3200_ICEPICK_SCR_FREERUNNINGTCK, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    //enable power/clock of TAP
    if(cc3200_icepick_router_command(1, CC3200_ICEPICK_SDTAP0_BLOCK, CC3200_ICEPICK_SDTAP0_REG,
            (CC3200_ICEPICK_SDTAP_FORCEACTIVE
                    | CC3200_ICEPICK_SDTAP_FORCEPOWER
                    | CC3200_ICEPICK_SDTAP_INHIBITSLEEP),
                    JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    //enable debug default mode
    if(cc3200_icepick_router_command(1, CC3200_ICEPICK_SDTAP0_BLOCK, CC3200_ICEPICK_SDTAP0_REG,
            (CC3200_ICEPICK_SDTAP_FORCEACTIVE
                    | CC3200_ICEPICK_SDTAP_FORCEPOWER
                    | CC3200_ICEPICK_SDTAP_INHIBITSLEEP
                    | CC3200_ICEPICK_SDTAP_DEBUGCONNECT),
                    JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    //select TAP
    if(cc3200_icepick_router_command(1, CC3200_ICEPICK_SDTAP0_BLOCK, CC3200_ICEPICK_SDTAP0_REG,
            (CC3200_ICEPICK_SDTAP_FORCEACTIVE
                    | CC3200_ICEPICK_SDTAP_FORCEPOWER
                    | CC3200_ICEPICK_SDTAP_INHIBITSLEEP
                    | CC3200_ICEPICK_SDTAP_DEBUGCONNECT
                    | CC3200_ICEPICK_SDTAP_TAPSELECT),
                    JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    //Delay for at least three clock cycles in RUN/IDLE state as per ICEPICK spec.
    jtag_scan_doStateMachine(0, 5);

    //set ICEPICK TAP to BYPASS
    if(jtag_scan_shiftIR(ICEPICK_IR_BYPASS, ICEPICK_IR_LEN, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    cc3200_icepick_state.configured = 1;
    return RET_SUCCESS;
}

int cc3200_icepick_warm_reset(void)
{
    if(!cc3200_icepick_state.initialized || !cc3200_icepick_state.detected || !cc3200_icepick_state.connected) {RETURN_ERROR(ERROR_UNKNOWN);}

    //Assert reset while keeping free-running TCK mode
    if(cc3200_icepick_router_command(1, CC3200_ICEPICK_SCR_BLOCK, CC3200_ICEPICK_SCR_REG,
            CC3200_ICEPICK_SCR_FREERUNNINGTCK | CC3200_ICEPICK_SCR_WARMRESET,
            JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}
