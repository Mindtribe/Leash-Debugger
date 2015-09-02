/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "jtag_scan.h"

#include <stdint.h>

#include "jtag_pinctl.h"
#include "jtag_statemachine.h"
#include "misc_hal.h"
#include "error.h"

//state struct
struct jtag_scan_state_t{
    unsigned char initialized;
    uint64_t shift_out;
};
//instantiate state struct
struct jtag_scan_state_t jtag_scan_state = {
    .initialized = 0,
    .shift_out = 0
};

int jtag_scan_init(void)
{
    if(jtag_scan_state.initialized) return RET_SUCCESS;

    if(jtag_pinctl_init() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    jtag_scan_state.initialized = 1;
    return RET_SUCCESS;
}

int jtag_scan_hardRst(void)
{
    if(!jtag_scan_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);}

    //hardware reset using the reset pin
    jtag_pinctl_assertPins(JTAG_RST);
    delay_loop(100000);
    jtag_pinctl_deAssertPins(JTAG_RST);
    delay_loop(20000000);

    return RET_SUCCESS;
}

int jtag_scan_rstStateMachine(void)
{
    if(!jtag_scan_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);}

    //five consecutive TMS 1's will reset the JTAG state machine of everything
    //in the chain.
    for(int i=0; i<5; i++) {
        jtag_pinctl_doClock(JTAG_TMS);
    }

    return RET_SUCCESS;
}

int jtag_scan_shiftDR(uint64_t data, uint32_t len, enum jtag_state_scan toState)
{
    if(!jtag_scan_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);}

    jtag_scan_state.shift_out = 0;

    //Get to Shift-DR state
    enum jtag_state cur_state = jtag_statemachine_getState();
    switch(cur_state){
    case JTAG_STATE_RTI:
    case JTAG_STATE_TLR:
        jtag_scan_doStateMachine(0x02, 4);
        break;
    case JTAG_STATE_PAUSEDR:
    case JTAG_STATE_PAUSEIR:
        jtag_scan_doStateMachine(0x07, 5);
        break;
    default:
        {RETURN_ERROR(ERROR_UNKNOWN);} //invalid state
        break;
    }

    //do shifting
    jtag_scan_doData(data, len);

    //get back to Run-Test/Idle state from current state (Exit-DR)
    //Get to Shift-DR state
    cur_state = jtag_statemachine_getState();
    switch(toState){
    case JTAG_STATE_SCAN_RUNIDLE:
        jtag_scan_doStateMachine(0x01, 2);
        if(jtag_statemachine_getState() != JTAG_STATE_RTI) RETURN_ERROR(jtag_statemachine_getState());
        break;
    case JTAG_STATE_SCAN_PAUSE:
        jtag_scan_doStateMachine(0x00, 1);
        if(jtag_statemachine_getState() != JTAG_STATE_PAUSEDR) RETURN_ERROR(jtag_statemachine_getState());
        break;
    default:
        {RETURN_ERROR(ERROR_UNKNOWN);} //invalid state
        break;
    }

    return RET_SUCCESS;
}

int jtag_scan_shiftIR(uint64_t data, uint32_t len, enum jtag_state_scan toState)
{
    if(!jtag_scan_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);}

    jtag_scan_state.shift_out = 0;

    //Get to Shift-IR state
    enum jtag_state cur_state = jtag_statemachine_getState();
    switch(cur_state){
    case JTAG_STATE_RTI:
    case JTAG_STATE_TLR:
        jtag_scan_doStateMachine(0x06, 5);
        break;
    case JTAG_STATE_PAUSEDR:
    case JTAG_STATE_PAUSEIR:
        jtag_scan_doStateMachine(0x0F, 6);
        break;
    default:
        {RETURN_ERROR(ERROR_UNKNOWN);} //invalid state
        break;
    }

    //do shifting
    jtag_scan_doData(data, len);

    //get back to Run-Test/Idle state from current state (Exit-IR)
    //Get to Shift-DR state
    switch(toState){
    case JTAG_STATE_SCAN_RUNIDLE:
        jtag_scan_doStateMachine(0x01, 2);
        if(jtag_statemachine_getState() != JTAG_STATE_RTI) {RETURN_ERROR(ERROR_UNKNOWN);}
        break;
    case JTAG_STATE_SCAN_PAUSE:
        jtag_scan_doStateMachine(0x00, 1);
        if(jtag_statemachine_getState() != JTAG_STATE_PAUSEIR) {RETURN_ERROR(ERROR_UNKNOWN);}
        break;
    default:
        {RETURN_ERROR(ERROR_UNKNOWN);} //invalid state
        break;
    }

    return RET_SUCCESS;
}

int jtag_scan_doStateMachine(uint32_t tms_bits_lsb_first, unsigned int num_clk)
{
    if(!jtag_scan_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(num_clk>=32) {RETURN_ERROR(ERROR_UNKNOWN);}

    for(unsigned int i=0; i<num_clk; i++){
        jtag_pinctl_doClock((tms_bits_lsb_first&(1<<i)) ? JTAG_TMS : JTAG_NONE);
    }

    return RET_SUCCESS;
}

int jtag_scan_doData(uint64_t tdi_bits_lsb_first, unsigned int num_clk)
{
    if(!jtag_scan_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(num_clk>64) {RETURN_ERROR(ERROR_UNKNOWN);}

    jtag_scan_state.shift_out = 0;

    for(unsigned int i=0; i<(num_clk-1); i++){
        jtag_pinctl_doClock((tdi_bits_lsb_first & 1) * JTAG_TDI);
        tdi_bits_lsb_first >>= 1;
        //jtag_pinctl_doClock((tdi_bits_lsb_first&(1<<i)) ? JTAG_TDI : JTAG_NONE);
        if(jtag_pinctl_getLastTDO()) jtag_scan_state.shift_out |= (((uint64_t)1)<<((uint64_t)i));
    }
    //last bit with TMS high
    jtag_pinctl_doClock(JTAG_TMS | (tdi_bits_lsb_first & 1) * JTAG_TDI);

    return RET_SUCCESS;
}

uint64_t jtag_scan_getShiftOut(void)
{
    return jtag_scan_state.shift_out;
}
