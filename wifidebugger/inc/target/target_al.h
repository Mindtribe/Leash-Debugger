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

enum stop_reason{
    STOPREASON_UNKNOWN = 0,
    STOPREASON_INTERRUPT,
    STOPREASON_BREAKPOINT
};

struct target_al_interface{
    //function pointers
    int (*target_init)(void);
    int (*target_reset)(void);
    int (*target_halt)(void);
    int (*target_continue)(void);
    int (*target_step)(void);
    int (*target_mem_read)(uint32_t, uint32_t*);
    int (*target_mem_write)(uint32_t, uint32_t);
    int (*target_mem_block_read)(uint32_t, uint32_t, uint8_t*);
    int (*target_mem_block_write)(uint32_t, uint32_t, uint8_t*);
    int (*target_get_gdb_reg_string)(char*);
    int (*target_put_gdb_reg_string)(char*);
    int (*target_set_pc)(uint32_t);
    int (*target_set_sw_bkpt)(uint32_t, uint8_t);
    int (*target_poll_halted)(uint8_t*);
    int (*target_handleHalt)(enum stop_reason *);
};

#endif
