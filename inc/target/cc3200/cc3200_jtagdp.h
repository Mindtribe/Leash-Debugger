/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef CC3200_JTAGDP_H_
#define CC3200_JTAGDP_H_

#include "jtag_scan.h"

//instruction register
#define CC3200_JTAGDP_IR_ABORT 0b1000
#define CC3200_JTAGDP_IR_DPACC 0b1010
#define CC3200_JTAGDP_IR_APACC 0b1011
#define CC3200_JTAGDP_IR_IDCODE 0b1110
#define CC3200_JTAGDP_IR_BYPASS 0b1111
#define CC3200_JTAGDP_IR_LEN 4

//data register
#define CC3200_JTAGDP_IDCODE_LEN 32
#define CC3200_JTAGDP_DPACC_LEN 35
#define CC3200_JTAGDP_APACC_LEN 35

//properties
#define CC3200_JTAGDP_DESIGNER_ARM 0x23B

//misc
#define CC3200_JTAGDP_OKFAULT 0b010
#define CC3200_JTAGDP_WAIT 0b001
#define CC3200_JTAGDP_ACC_RETRIES 5
#define CC3200_JTAGDP_PWRUP_RETRIES 5

//control/status register
#define CC3200_JTAGDP_CSYSPWRUPACK (1<<31)
#define CC3200_JTAGDP_CSYSPWRUPREQ (1<<30)
#define CC3200_JTAGDP_CDBGPWRUPACK (1<<29)
#define CC3200_JTAGDP_CDBGPWRUPREQ (1<<28)
#define CC3200_JTAGDP_CDBGRSTACK (1<<27)
#define CC3200_JTAGDP_CDBGRSTREQ (1<<26)
#define CC3200_JTAGDP_TRNCNT_OFFSET 12
#define CC3200_JTAGDP_TRNCNT_MASK (0x3FF<<CC3200_JTAGDP_TRNCNT_OFFSET)
#define CC3200_JTAGDP_MASKLANE_OFFSET 8
#define CC3200_JTAGDP_MASKLANE_MASK (0xF<<CC3200_JTAGDP_MASKLANE_OFFSET)
#define CC3200_JTAGDP_WDATAERR (1<<7)
#define CC3200_JTAGDP_READOK (1<<6)
#define CC3200_JTAGDP_STICKYERR (1<<5)
#define CC3200_JTAGDP_STICKYCMP (1<<4)
#define CC3200_JTAGDP_TRNMODE_OFFSET 2
#define CC3200_JTAGDP_TRNMODE_MASK (0x03<<CC3200_JTAGDP_TRNMODE_OFFSET)
#define CC3200_JTAGDP_STICKYORUN (1<<1)
#define CC3200_JTAGDP_ORUNDETECT (1)

//initialize the jtagdp state.
//this represents the "JTAGDP" as specified in the ARM debug interface architecture (one layer deeper than TI's ICEPICK router).
//Therefore it is not the first thing in the scan chain - the ICEPICK is always in front on the CC3200.
//This module does assume it is the LAST thing in the scan chain.
//Arguments to this function allow to configure how many preceding IR and DR bits there are in the scan chain, and what their
//values should be set to when trying to access the JTAGDP (normally, 6 bits IR set to 1 for BYPASSing the ICEPICK, and
//1 bit with dont-care value for going through the 1-bit BYPASS DR of the ICEPICK).
int cc3200_jtagdp_init(int num_precede_ir_bits, uint64_t precede_ir_bits, int num_precede_dr_bits, uint64_t precede_dr_bits);

//attempt to detect the JTAGDP of the ARM core. Note that all ICEPICK configuration must have been done beforehand.
int cc3200_jtagdp_detect(void);

//wrapper for shiftIR to access JTAG-DP port only
int cc3200_jtagdp_shiftIR(uint8_t data, enum jtag_state_scan fromState, enum jtag_state_scan toState);

//wrapper for shiftDR to access JTAG-DP port only
int cc3200_jtagdp_shiftDR(uint64_t data, int DR_len, enum jtag_state_scan fromState, enum jtag_state_scan toState);

//get the response of the last transaction.
int cc3200_jtagdp_accResponse(uint8_t* response, enum jtag_state_scan fromState, enum jtag_state_scan toState);

//perform a DPACC write on the JTAG-DP
int cc3200_jtagdp_DPACC_write(uint8_t addr, uint32_t value);

//perform a DPACC read on the JTAG-DP
int cc3200_jtagdp_DPACC_read(uint8_t addr, uint32_t* result);

//perform an APACC write on the JTAG-DP
int cc3200_jtagdp_APACC_write(uint8_t addr, uint32_t value);

//perform an APACC read on the JTAG-DP
int cc3200_jtagdp_APACC_read(uint8_t addr, uint32_t* result);

#endif
