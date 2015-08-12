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

#define IDCODE_MANUFACTURER_TI 0x17
//note: this part number was read from the device, and online resources suggest
//that this is indeed the part number of a CC3200. However, no official documentation
//has been found that gives any clue whether CC3200's might exist with other
//part numbers. To be determined.
#define IDCODE_PARTNUMBER_CC3200 0xB97C

#define ICEPICKCODE_TYPE_C 0x1CC

//Icepick-level JTAG instructions
#define ICEPICK_IR_LEN 6
#define ICEPICK_IR_IDCODE 0b000100
#define ICEPICK_IR_ICEPICKCODE 0b000101
#define ICEPICK_IR_BYPASS 0b111111
#define ICEPICK_IR_ROUTER 0b000010
#define ICEPICK_IR_CONNECT 0b000111

struct cc3200_icepick_properties_t{
    uint16_t IDCODE_MANUFACTURER;
    uint16_t IDCODE_PARTNUMBER;
    uint8_t IDCODE_VERSION;
    uint8_t ICEPICKCODE_MAJORVERSION;
    uint8_t ICEPICKCODE_MINORVERSION;
    uint8_t ICEPICKCODE_TESTTAPS;
    uint8_t ICEPICKCODE_EMUTAPS;
    uint16_t ICEPICKCODE_ICEPICKTYPE;
    uint8_t ICEPICKCODE_CAPABILITIES;
};

//initialize the JTAG interface to the CC3200 with ICEPICK.
int cc3200_icepick_init(void);

//read IDCODE and ICEPICKCODE to get properties of target.
int cc3200_icepick_detect(void);

//connect the ICEPICK to the rest of the chip's debug hardware
int cc3200_icepick_connect(void);

//disconnect the ICEPICK from the rest of the chip's debug hardware
int cc3200_icepick_disconnect(void);

//send the ICEPICK a ROUTER-command.
int cc3200_icepick_router_command(uint8_t rw, uint8_t block, uint8_t reg, uint32_t value, enum jtag_state_scan fromState, enum jtag_state_scan toState);

//configure the ICEPICK for debugging the CC3200's core TAP.
int cc3200_icepick_configure(void);

//TODO: possibly add full system reset via ICEPICK module (see ICEPICK reference manual for docs on this feature)

#endif
