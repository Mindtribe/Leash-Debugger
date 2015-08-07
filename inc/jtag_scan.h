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

#include <stdint.h>

//Initializer - call once before using.
int jtag_scan_init(void);

//Perform a hardware reset on target using RST line.
int jtag_scan_hardRst(void);

//Reset the TAP access port state machine using TMS signaling.
int jtag_scan_rstStateMachine(void);

//Shift data into/out of Data Register.
int jtag_scan_shiftDR(uint32_t data, uint32_t len);

//Shift data into/out of Instruction Register.
int jtag_scan_shiftIR(uint32_t data, uint32_t len);

//Get the shifted out value from the most recent scan operation.
uint32_t jtag_scan_getShiftOut(void);
