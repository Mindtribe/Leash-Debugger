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
#include "jtag_codes.h"
#include "common.h"

//state struct
struct cc3200_icepick_state_t{
    unsigned char initialized;
    unsigned char detected;
    struct cc3200_icepick_properties_t properties;

};
//instantiate state struct
struct cc3200_icepick_state_t cc3200_icepick_state = {
    .initialized = 0,
    .detected = 0,
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

    if(jtag_scan_init() == RET_FAILURE) return RET_FAILURE;

    cc3200_icepick_state.initialized = 1;
    return RET_SUCCESS;
}

int cc3200_icepick_detect(void)
{
    if(!cc3200_icepick_state.initialized) return RET_FAILURE;

    jtag_scan_hardRst();
    jtag_scan_rstStateMachine();

    jtag_scan_shiftIR(ICEPICK_IDCODE, ICEPICK_INST_LEN);
    jtag_scan_shiftDR(0,32); //get 32-bit IDCODE
    uint32_t idcode = jtag_scan_getShiftOut();

    cc3200_icepick_state.properties.IDCODE_MANUFACTURER = (uint16_t) ((idcode>>1) & 0x3FF);
    cc3200_icepick_state.properties.IDCODE_PARTNUMBER = (uint16_t) ((idcode>>12) & 0xFFFF);
    cc3200_icepick_state.properties.IDCODE_VERSION = (uint8_t) (idcode>>28);

    //check for validity
    if( (cc3200_icepick_state.properties.IDCODE_MANUFACTURER != IDCODE_MANUFACTURER_TI) ||
            (cc3200_icepick_state.properties.IDCODE_PARTNUMBER != IDCODE_PARTNUMBER_CC3200) ){
        cc3200_icepick_state.detected = 0;
        return RET_FAILURE;
    }

    jtag_scan_shiftIR(ICEPICK_ICEPICKCODE, ICEPICK_INST_LEN);
    jtag_scan_shiftDR(0,32); //get 32-bit ICEPICKCODE
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
        return RET_FAILURE;
    }

    cc3200_icepick_state.detected = 1;
    return RET_SUCCESS;
}
