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
#include "misc_hal.h"
#include "error.h"

enum jtag_state{
    JTAG_STATE_TLR = 0,
    JTAG_STATE_RTI,
    JTAG_STATE_DRSCAN,
    JTAG_STATE_CAPTDR,
    JTAG_STATE_SHIFTDR,
    JTAG_STATE_EXIT1DR,
    JTAG_STATE_PAUSEDR,
    JTAG_STATE_EXIT2DR,
    JTAG_STATE_UPDATEDR,
    JTAG_STATE_IRSCAN,
    JTAG_STATE_CAPTIR,
    JTAG_STATE_SHIFTIR,
    JTAG_STATE_EXIT1IR,
    JTAG_STATE_PAUSEIR,
    JTAG_STATE_EXIT2IR,
    JTAG_STATE_UPDATEIR
};

//state struct
struct jtag_scan_state_t{
    unsigned char initialized;
    enum jtag_state cur_jtag_state;
};
//instantiate state struct
struct jtag_scan_state_t jtag_scan_state = {
    .initialized = 0,
    .cur_jtag_state = 0
};

int jtag_scan_init(void)
{
    if(jtag_scan_state.initialized) return RET_SUCCESS;

    if(jtag_pinctl_init() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "JTAG pin init fail");}

    jtag_scan_state.cur_jtag_state = JTAG_STATE_TLR;

    jtag_scan_state.initialized = 1;
    return RET_SUCCESS;
}

int jtag_scan_deinit(void)
{
    jtag_scan_state.initialized = 0;
    return RET_SUCCESS;
}

int jtag_scan_hardRst(void)
{
    if(!jtag_scan_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    //hardware reset using the reset pin
    jtag_pinctl_assertPins(JTAG_RST);
    delay_loop(10000000);
    jtag_pinctl_deAssertPins(JTAG_RST);
    delay_loop(20000000);

    jtag_scan_state.cur_jtag_state = JTAG_STATE_TLR;

    return RET_SUCCESS;
}

int jtag_scan_rstStateMachine(void)
{
    if(!jtag_scan_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    //five consecutive TMS 1's will reset the JTAG state machine of everything
    //in the chain.
    for(int i=0; i<5; i++) {
        uint8_t dummy;
        jtag_pinctl_doClock(1,0,&dummy);
    }

    jtag_scan_state.cur_jtag_state = JTAG_STATE_TLR;

    return RET_SUCCESS;
}

int jtag_scan_shiftDR(uint64_t data, uint32_t len, enum jtag_state_scan toState)
{
    if(!jtag_scan_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    //Get to Shift-DR state
    switch(jtag_scan_state.cur_jtag_state){
    case JTAG_STATE_RTI:
    case JTAG_STATE_TLR:
        jtag_pinctl_doStateMachine(0x02, 4);
        break;
    case JTAG_STATE_PAUSEDR:
    case JTAG_STATE_PAUSEIR:
        jtag_pinctl_doStateMachine(0x07, 5);
        break;
    default:
        RETURN_ERROR(ERROR_UNKNOWN, "JTAG state fail"); //invalid state
        break;
    }

    //do shifting
    jtag_pinctl_doData(data, len);

    //get back to Run-Test/Idle state from current state (Exit-DR)
    //Get to Shift-DR state
    switch(toState){
    case JTAG_STATE_SCAN_RUNIDLE:
        jtag_pinctl_doStateMachine(0x01, 2);
        jtag_scan_state.cur_jtag_state = JTAG_STATE_RTI;
        break;
    case JTAG_STATE_SCAN_PAUSE:
        jtag_pinctl_doStateMachine(0x00, 1);
        jtag_scan_state.cur_jtag_state = JTAG_STATE_PAUSEDR;
        break;
    default:
        RETURN_ERROR(ERROR_UNKNOWN, "JTAG state fail"); //invalid state
        break;
    }

    return RET_SUCCESS;
}

int jtag_scan_shiftIR(uint64_t data, uint32_t len, enum jtag_state_scan toState)
{
    if(!jtag_scan_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    //Get to Shift-IR state
    switch(jtag_scan_state.cur_jtag_state){
    case JTAG_STATE_RTI:
    case JTAG_STATE_TLR:
        jtag_pinctl_doStateMachine(0x06, 5);
        break;
    case JTAG_STATE_PAUSEDR:
    case JTAG_STATE_PAUSEIR:
        jtag_pinctl_doStateMachine(0x0F, 6);
        break;
    default:
        RETURN_ERROR(ERROR_UNKNOWN, "JTAG state fail"); //invalid state
        break;
    }

    //do shifting
    jtag_pinctl_doData(data, len);

    //get back to Run-Test/Idle state from current state (Exit-IR)
    //Get to Shift-DR state
    switch(toState){
    case JTAG_STATE_SCAN_RUNIDLE:
        jtag_pinctl_doStateMachine(0x01, 2);
        jtag_scan_state.cur_jtag_state = JTAG_STATE_RTI;
        break;
    case JTAG_STATE_SCAN_PAUSE:
        jtag_pinctl_doStateMachine(0x00, 1);
        jtag_scan_state.cur_jtag_state = JTAG_STATE_PAUSEIR;
        break;
    default:
        RETURN_ERROR(ERROR_UNKNOWN, "JTAG state fail"); //invalid state
        break;
    }

    return RET_SUCCESS;
}

uint64_t jtag_scan_getShiftOut(void)
{
    return jtag_pinctl_scanout;
}
