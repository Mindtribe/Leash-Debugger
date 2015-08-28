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
    STOPREASON_BREAKPOINT,
    STOPREASON_SEMIHOSTING
};

enum semihost_opcode{
    SEMIHOST_READCONSOLE = 0,
    SEMIHOST_WRITECONSOLE,
    SEMIHOST_UNKNOWN
};

struct semihost_operation{
    enum semihost_opcode opcode;
    uint32_t param1;
    uint32_t param2;
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
    int (*target_read_register)(uint8_t, uint32_t*);
    int (*target_write_register)(uint8_t, uint32_t);
    int (*target_set_pc)(uint32_t);
    int (*target_get_pc)(uint32_t*);
    int (*target_set_sw_bkpt)(uint32_t, uint8_t);
    int (*target_poll_halted)(uint8_t*);
    int (*target_handleHalt)(enum stop_reason *);
    int (*target_querySemiHostOp)(struct semihost_operation *op);
};

#endif
