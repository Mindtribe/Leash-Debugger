/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include <stdint.h>

#include "jtag_scan.h"
#include "jtag_pinctl.h"
#include "common.h"

//state struct
struct jtag_scan_state_t{
    unsigned char initialized;
    uint32_t shift_out;
};
//instantiate state struct
struct jtag_scan_state_t jtag_scan_state = {
    .initialized = 0,
    .shift_out = 0
};

int jtag_scan_init(void)
{
    if(jtag_scan_state.initialized) return RET_SUCCESS;

    if(jtag_pinctl_init() == RET_FAILURE) return RET_FAILURE;

    jtag_scan_state.initialized = 1;
    return RET_SUCCESS;
}

int jtag_scan_hardRst(void)
{
    if(!jtag_scan_state.initialized) return RET_FAILURE;

    //hardware reset using the reset pin (repeatedly for longer pulse)
    for(int i=0; i<10; i++){
        jtag_pinctl_doClock(JTAG_RST);
    }
    jtag_pinctl_doClock(JTAG_NONE); //end pulse

    return RET_SUCCESS;
}

int jtag_scan_rstStateMachine(void)
{
    if(!jtag_scan_state.initialized) return RET_FAILURE;

    //five consequtive TMS 1's will reset the JTAG state machine of everything
    //in the chain.
    for(int i=0; i<5; i++) jtag_pinctl_doClock(JTAG_TMS);

    return RET_SUCCESS;
}

int jtag_scan_shiftDR(uint32_t data, uint32_t len)
{
    if(!jtag_scan_state.initialized) return RET_FAILURE;

    jtag_scan_state.shift_out = 0;

    //Get to Shift-DR state
    jtag_pinctl_doClock(JTAG_NONE);
    jtag_pinctl_doClock(JTAG_TMS);
    jtag_pinctl_doClock(JTAG_NONE);
    jtag_pinctl_doClock(JTAG_NONE);

    //do shifting
    for(int i=0; i<len; i++){
        jtag_pinctl_doClock((data & 1) * JTAG_TDI); //send LSB
        data = data>>1;
        jtag_scan_state.shift_out |= jtag_pinctl_getLastTDO() * (1<<(len-i-1));
    }

    //get back to Run-Test/Idle state
    jtag_pinctl_doClock(JTAG_TMS);
    jtag_pinctl_doClock(JTAG_NONE);
    jtag_pinctl_doClock(JTAG_TMS);
    jtag_pinctl_doClock(JTAG_TMS);
    jtag_pinctl_doClock(JTAG_NONE);

    return RET_SUCCESS;
}

int jtag_scan_shiftIR(uint32_t data, uint32_t len)
{
    if(!jtag_scan_state.initialized) return RET_FAILURE;

    jtag_scan_state.shift_out = 0;

    //Get to Shift-IR state
    jtag_pinctl_doClock(JTAG_NONE);
    jtag_pinctl_doClock(JTAG_TMS);
    jtag_pinctl_doClock(JTAG_TMS);
    jtag_pinctl_doClock(JTAG_NONE);
    jtag_pinctl_doClock(JTAG_NONE);

    //do shifting
    for(int i=0; i<len; i++){
        jtag_pinctl_doClock((data & 1) * JTAG_TDI); //send LSB
        data = data>>1;
        jtag_scan_state.shift_out |= jtag_pinctl_getLastTDO() * (1<<i);
    }

    //get back to Run-Test/Idle state
    jtag_pinctl_doClock(JTAG_TMS);
    jtag_pinctl_doClock(JTAG_NONE);
    jtag_pinctl_doClock(JTAG_TMS);
    jtag_pinctl_doClock(JTAG_TMS);
    jtag_pinctl_doClock(JTAG_NONE);

    return RET_SUCCESS;
}

uint32_t jtag_scan_getShiftOut(void)
{
    return jtag_scan_state.shift_out;
}
