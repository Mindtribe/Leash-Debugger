/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */


#include "jtag_statemachine.h"

struct jtag_statemachine_state_t{
    enum jtag_state state;
};

struct jtag_statemachine_state_t jtag_statemachine_state = {
    .state = JTAG_STATE_TLR
};

enum jtag_state jtag_statemachine_getState(void)
{
    return jtag_statemachine_state.state;
}

void jtag_statemachine_transition(uint8_t TMS)
{
    switch(jtag_statemachine_state.state){
    case JTAG_STATE_TLR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_TLR : JTAG_STATE_RTI;
        break;
    case JTAG_STATE_RTI:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_DRSCAN : JTAG_STATE_RTI;
        break;
    case JTAG_STATE_DRSCAN:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_IRSCAN : JTAG_STATE_CAPTDR;
        break;
    case JTAG_STATE_IRSCAN:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_TLR : JTAG_STATE_CAPTIR;
        break;
    case JTAG_STATE_CAPTDR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_EXIT1DR : JTAG_STATE_SHIFTDR;
        break;
    case JTAG_STATE_CAPTIR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_EXIT1IR : JTAG_STATE_SHIFTIR;
        break;
    case JTAG_STATE_SHIFTDR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_EXIT1DR : JTAG_STATE_SHIFTDR;
        break;
    case JTAG_STATE_SHIFTIR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_EXIT1IR : JTAG_STATE_SHIFTIR;
        break;
    case JTAG_STATE_EXIT1DR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_UPDATEDR : JTAG_STATE_PAUSEDR;
        break;
    case JTAG_STATE_EXIT1IR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_UPDATEIR : JTAG_STATE_PAUSEIR;
        break;
    case JTAG_STATE_PAUSEDR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_EXIT2DR : JTAG_STATE_PAUSEDR;
        break;
    case JTAG_STATE_PAUSEIR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_EXIT2DR : JTAG_STATE_PAUSEIR;
        break;
    case JTAG_STATE_EXIT2DR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_UPDATEDR : JTAG_STATE_SHIFTDR;
        break;
    case JTAG_STATE_EXIT2IR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_UPDATEIR : JTAG_STATE_SHIFTIR;
        break;
    case JTAG_STATE_UPDATEDR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_DRSCAN : JTAG_STATE_RTI;
        break;
    case JTAG_STATE_UPDATEIR:
        jtag_statemachine_state.state = TMS ? JTAG_STATE_DRSCAN : JTAG_STATE_RTI;
        break;
    default:
        break;
    }
}

void jtag_statemachine_reset(void)
{
    jtag_statemachine_state.state = JTAG_STATE_TLR;
}

void jtag_statemachine_setState(enum jtag_state newstate)
{
    jtag_statemachine_state.state = newstate;
}
