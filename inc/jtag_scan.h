/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

/*
For performing basic composite JTAG operations like read/write data/instruction registers.
 */

#ifndef JTAG_SCAN_H_
#define JTAG_SCAN_H_

#include <stdint.h>

//jtag states. This does not have all states of the state machine, only
//those that are interesting to start from or return to when doing
//IR or DR scan operations.
enum jtag_state{
    JTAG_STATE_PAUSE = 0,
    JTAG_STATE_RUNIDLE
};

//Initializer - call once before using.
int jtag_scan_init(void);

//Perform a hardware reset on target using RST line.
int jtag_scan_hardRst(void);

//Reset the TAP access port state machine using TMS signaling.
int jtag_scan_rstStateMachine(void);

//Perform a sequence of TMS assertions to move through the JTAG state machine.
int jtag_scan_doStateMachine(uint32_t tms_bits_lsb_first, unsigned int num_clk);

//Shift data bits (TDI line) into the JTAG interface. State machine is assumed to be in correct state.
//On the last bit shifted, TMS is made high to exit whatever shift state JTAG is in.
int jtag_scan_doData(uint64_t tdi_bits_lsb_first, unsigned int num_clk);

//Shift data into/out of Data Register.
int jtag_scan_shiftDR(uint32_t data, uint32_t len, enum jtag_state fromState, enum jtag_state toState);

//Shift data into/out of Instruction Register.
int jtag_scan_shiftIR(uint32_t data, uint32_t len, enum jtag_state fromState, enum jtag_state toState);

//Get the shifted out value from the most recent scan operation.
uint64_t jtag_scan_getShiftOut(void);

#endif
