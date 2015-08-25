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

//properties to check
#define CC3200_CORE_AP_IDR 0x24770011
#define CC3200_CORE_COMPID 0xB105100D
#define CC3200_CORE_PERID 0x4000bb4c4

//AP addresses
#define CC3200_CORE_AP_CSW_ADDR 0x00
#define CC3200_CORE_AP_TRANSFERADDR_ADDR 0x04
#define CC3200_CORE_AP_BD0_ADDR 0x10
#define CC3200_CORE_AP_BD1_ADDR 0x14
#define CC3200_CORE_AP_BD2_ADDR 0x18
#define CC3200_CORE_AP_BD3_ADDR 0x1C
#define CC3200_CORE_AP_DATARW_ADDR 0x0C
#define CC3200_CORE_AP_CFG_ADDR 0xF4
#define CC3200_CORE_AP_BASE_ADDR 0xF8
#define CC3200_CORE_AP_IDR_ADDR 0xFC

//bitmasks in control/status register
#define CC3200_CORE_CSW_DBGSWENABLE (1<<31)
#define CC3200_CORE_CSW_PROT_OFFSET (24)
#define CC3200_CORE_CSW_PROT_MASK (0x7F << CC3200_CORE_CSW_PROT_OFFSET)
#define CC3200_CORE_CSW_SPIDEN (1<<23)
#define CC3200_CORE_CSW_MODE_OFFSET (8)
#define CC3200_CORE_CSW_MODE_MASK (0xF << CC3200_CORE_CSW_MODE_OFFSET)
#define CC3200_CORE_CSW_TRINPROG (1<<7)
#define CC3200_CORE_CSW_DEVICEEN (1<<6)
#define CC3200_CORE_CSW_ADDRINC_OFFSET (4)
#define CC3200_CORE_CSW_ADDRINC_MASK (0x3 << CC3200_CORE_CSW_ADDRINC_OFFSET)
#define CC3200_CORE_CSW_SIZE_OFFSET (0)
#define CC3200_CORE_CSW_SIZE_MASK (0x7 << CC3200_CORE_CSW_SIZE_OFFSET)

//settings in control/status register
#define CC3200_CORE_CSW_ADDRINC_SINGLE (1 << CC3200_CORE_CSW_ADDRINC_OFFSET)

//addresses relative to DEBUG_BASE
#define CC3200_CORE_BASEOFFSET_COMPID0 0xFF0
#define CC3200_CORE_BASEOFFSET_COMPID1 0xFF4
#define CC3200_CORE_BASEOFFSET_COMPID2 0xFF8
#define CC3200_CORE_BASEOFFSET_COMPID3 0xFFC
#define CC3200_CORE_BASEOFFSET_PERID0 0xFE0
#define CC3200_CORE_BASEOFFSET_PERID1 0xFE4
#define CC3200_CORE_BASEOFFSET_PERID2 0xFE8
#define CC3200_CORE_BASEOFFSET_PERID3 0xFEC
#define CC3200_CORE_BASEOFFSET_PERID4 0xFD0
#define CC3200_CORE_BASEOFFSET_MEMTYPE 0xFCC

//SCS
#define CC3200_CORE_MEM_DFSR 0xD30
#define CC3200_CORE_MEM_DHCSR 0xDF0
#define CC3200_CORE_MEM_DCRSR 0xDF4
#define CC3200_CORE_MEM_DCRDR 0xDF8
#define CC3200_CORE_MEM_DEMCR 0xDFC

//DHCSR bitmasks
#define CC3200_CORE_MEM_DHCSR_C_DEBUGEN (1<<0)
#define CC3200_CORE_MEM_DHCSR_C_HALT (1<<1)
#define CC3200_CORE_MEM_DHCSR_C_STEP (1<<2)
#define CC3200_CORE_MEM_DHCSR_S_REGRDY (1<<16)
#define CC3200_CORE_MEM_DHCSR_S_HALT (1<<17)
#define CC3200_CORE_MEM_DHCSR_DBGKEY_OFFSET 16
#define CC3200_CORE_MEM_DHCSR_DBGKEY_MASK (0xFFFF<<CC3200_CORE_MEM_DHCSR_DBGKEY_OFFSET)

//DCRSR bitmasks
#define CC3200_CORE_MEM_DCRSR_REGWNR (1<<16)
#define CC3200_CORE_MEM_REGSEL_MASK 0x1F

//DFSR bitmasks
#define CC3200_CORE_DFSR_BKPT (1<<1)

//misc
#define CC3200_CORE_DBGKEY 0xA05F //"key" to unlock debug halting register writes
#define CC3200_CORE_CSW_SIZE_WORD 0x02 //this setting of SIZE bits in MEM-AP CSW enables 32-bit memory accesses.

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

#endif
