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

#define CC3200_OPCODE_BKPT 0xBE00

#define CC3200_SEMIHOST_WRITE0 0x04
#define CC3200_SEMIHOST_READ 0x06

extern struct target_al_interface cc3200_interface;

int cc3200_init(void);

int cc3200_reset(void);

int cc3200_halt(void);

int cc3200_continue(void);

int cc3200_step(void);

int cc3200_mem_read(uint32_t addr, uint32_t* dst);

int cc3200_mem_write(uint32_t addr, uint32_t value);

int cc3200_mem_block_read(uint32_t addr, uint32_t bytes, uint8_t *dst);

int cc3200_mem_block_write(uint32_t addr, uint32_t bytes, uint8_t *src);

int cc3200_reg_read(uint8_t regnum, uint32_t *dst);

int cc3200_reg_write(uint8_t regnum, uint32_t value);

int cc3200_get_gdb_reg_string(char* string);

int cc3200_put_gdb_reg_string(char* string);

int cc3200_set_pc(uint32_t addr);

int cc3200_set_sw_bkpt(uint32_t addr, uint8_t len_bytes);

int cc3200_poll_halted(uint8_t *result);

int cc3200_handleHalt(enum stop_reason *reason);

int cc3200_get_pc(uint32_t* dst);

int cc3200_querySemiHostOp(struct semihost_operation *op);

#endif
