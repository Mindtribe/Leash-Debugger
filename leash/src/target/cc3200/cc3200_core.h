/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

/*
 * Functions to interact with the Cortex M4's access ports through its JTAG-DP.
 * Written for (and only tested on) the CC3200, but should be mostly compatible with Cortex M4 in general.
 */

#ifndef CC3200_CORE_H_
#define CC3200_CORE_H_

#include <stdint.h>
#include "cc3200_reg.h"

//initialize the core access port. Assumes cc3200_jtagdp is initialized already.
int cc3200_core_init(void);

//detect the core access port. Assumes this module is initialized already.
int cc3200_core_detect(void);

//read a register of the access port. regaddr is the combined bank and register address.
int cc3200_core_read_APreg(uint8_t ap, uint8_t regaddr, uint32_t* result, uint8_t check_response);

//write a register of the access port. regaddr is the combined bank and register address.
int cc3200_core_write_APreg(uint8_t ap, uint8_t regaddr, uint32_t value, uint8_t check_response);

//pipelined writes to register of the access port. regaddr is the combined bank and register address.
int cc3200_core_pipeline_write_APreg(uint8_t ap, uint8_t regaddr, uint32_t len, uint32_t *values);

//pipelined reads from register of the access port. regaddr is the combined bank and register address.
int cc3200_core_pipeline_read_APreg(uint8_t ap, uint8_t regaddr, uint32_t len, uint32_t *dst);

//read a memory location from the core system memory space.
int cc3200_core_read_mem_addr(uint32_t addr, uint32_t* result);

//write a memory location from the core system memory space.
int cc3200_core_write_mem_addr(uint32_t addr, uint32_t value);

//write successive memory locations in a pipelined fashion.
int cc3200_core_pipeline_write_mem_addr(uint32_t addr, uint32_t len, uint32_t* values);

//read successive memory locations in a pipelined fashion.
int cc3200_core_pipeline_read_mem_addr(uint32_t addr, uint32_t len, uint32_t *dst);

//halt the core.
int cc3200_core_debug_halt(void);

//continue execution.
int cc3200_core_debug_continue(void);

//step an instruction
int cc3200_core_debug_step(void);

//enable debug.
int cc3200_core_debug_enable(void);

//get the debug base address.
uint32_t cc3200_core_get_debug_base(void);

//read a register.
int cc3200_core_read_reg(enum cc3200_reg_index reg, uint32_t* dst);

//write a register.
int cc3200_core_write_reg(enum cc3200_reg_index reg, uint32_t value);

//set the program counter.
int cc3200_core_set_pc(uint32_t addr);

//poll to see if the target is halted
int cc3200_core_poll_halted(uint8_t *result);

//get the DFSR register to examine halt reason
int cc3200_core_getDFSR(uint32_t *result);

//get the program counter.
int cc3200_core_get_pc(uint32_t *dst);

//set the reset vector catch, and then do a local core reset, waiting until the processor halts upon entering the reset vector.
int cc3200_core_debug_reset_halt(void);

#endif
