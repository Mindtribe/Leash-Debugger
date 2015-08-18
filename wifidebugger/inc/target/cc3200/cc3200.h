/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

/*
 * The encompassing interface header for the CC3200 target.
 */

#ifndef CC3200_H_
#define CC3200_H_

#include "target_al.h"

extern struct target_al_interface cc3200_interface;

int cc3200_init(void);

int cc3200_reset(void);

int cc3200_halt(void);

int cc3200_continue(void);

int cc3200_mem_read(uint32_t addr, uint32_t* dst);

int cc3200_mem_write(uint32_t addr, uint32_t value);

#endif
