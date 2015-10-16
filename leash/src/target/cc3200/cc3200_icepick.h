/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

/*
 * Functions to interact with the ICEPICK type C JTAG router found in CC3200 SoCs.
 */

#ifndef CC3200_ICEPICK_H_
#define CC3200_ICEPICK_H_

#include <stdint.h>

#include "jtag_scan.h"

//Icepick-level JTAG instructions
#define ICEPICK_IR_LEN 6
#define ICEPICK_IR_IDCODE 0x04
#define ICEPICK_IR_ICEPICKCODE 0x05
#define ICEPICK_IR_BYPASS 0x3F
#define ICEPICK_IR_ROUTER 0x02
#define ICEPICK_IR_CONNECT 0x07

//initialize the JTAG interface to the CC3200 with ICEPICK.
int cc3200_icepick_init(void);

//de-initialize. After this, the module may be re-initialized using cc3200_icepick_init().
int cc3200_icepick_deinit(void);

//read IDCODE and ICEPICKCODE to get properties of target.
int cc3200_icepick_detect(void);

//connect the ICEPICK to the rest of the chip's debug hardware
int cc3200_icepick_connect(void);

//disconnect the ICEPICK from the rest of the chip's debug hardware
int cc3200_icepick_disconnect(void);

//send the ICEPICK a ROUTER-command.
int cc3200_icepick_router_command(uint8_t rw, uint8_t block, uint8_t reg, uint32_t value, enum jtag_state_scan toState);

//configure the ICEPICK for debugging the CC3200's core TAP.
int cc3200_icepick_configure(void);

//perform a "warm system reset"
int cc3200_icepick_warm_reset(void);

#endif
