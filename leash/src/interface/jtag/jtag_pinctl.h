/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

/*
Abstraction layer from platform-specific GPIO functions when
performing the most basic logical operations of 4-wire JTAG interface.
 */

#ifndef JTAG_PINCTL_H_
#define JTAG_PINCTL_H_

#include <stdint.h>

//this enum can be used for the "pins" arguments of this module's API functions.
enum jtagPinBit{
    JTAG_NONE = 0,
    JTAG_TDI = 1,
    JTAG_TMS = 2,
    JTAG_RST = 4,
    JTAG_TCK = 8
};

extern uint64_t jtag_pinctl_scanout;

//Initializer - should be called once, before any other calls to this API.
int jtag_pinctl_init(void);

//Generate a TCK clock period, with the specified pin(s) active.
void jtag_pinctl_doClock(uint8_t TMS, uint8_t TDI, uint8_t* TDO_result);

//for direct control
int jtag_pinctl_assertPins(uint8_t pins);
int jtag_pinctl_deAssertPins(uint8_t pins);

//for doing a sequence of clocks on the state machine or data line, respectively
void jtag_pinctl_doStateMachine(uint32_t tms_bits_lsb_first, unsigned int num_clk);
void jtag_pinctl_doData(uint64_t tdi_bits_lsb_first, unsigned int num_clk);

#endif
