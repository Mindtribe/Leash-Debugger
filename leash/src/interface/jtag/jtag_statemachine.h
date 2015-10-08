/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

/*
 * This is meant as a sanity check for the JTAG code that actually performs operations.
 * It can be used to keep track of the current state in the JTAG state machine.
 * It does not perform any kind of real JTAG operation.
 */

#ifndef JTAG_STATEMACHINE_H_
#define JTAG_STATEMACHINE_H_

#include <stdint.h>

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

enum jtag_state jtag_statemachine_getState(void);

void jtag_statemachine_transition(uint8_t TMS);

void jtag_statemachine_reset(void);

void jtag_statemachine_setState(enum jtag_state newstate);

#endif
