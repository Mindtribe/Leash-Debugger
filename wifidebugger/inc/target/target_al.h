/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

/*
 * For abstracting from different debug targets to generic debugging calls.
 */

#ifndef TARGET_AL_H_
#define TARGET_AL_H_

#include <stdint.h>

struct target_al_interface{
    int (*target_init)(void);
    int (*target_reset)(void);
    int (*target_halt)(void);
    int (*target_continue)(void);
    int (*target_mem_read)(uint32_t, uint32_t*);
    int (*target_mem_write)(uint32_t, uint32_t);
};

#endif