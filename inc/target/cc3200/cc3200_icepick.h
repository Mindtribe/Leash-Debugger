/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef CC3200_ICEPICK_H_
#define CC3200_ICEPICK_H_

#include <stdint.h>

#define IDCODE_MANUFACTURER_TI 0x17
//note: this part number was read from the device, and online resources suggest
//that this is indeed the part number of a CC3200. However, no official documentation
//has been found that gives any clue whether CC3200's might exist with other
//part numbers. To be determined.
#define IDCODE_PARTNUMBER_CC3200 0xB97C

#define ICEPICKCODE_TYPE_C 0x1CC

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

#endif
