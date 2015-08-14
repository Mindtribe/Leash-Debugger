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

//properties to check
#define CC3200_CORE_AP_IDR 0x24770011
#define CC3200_CORE_COMPID 0xB105100D
#define CC3200_CORE_PERID 0x4000bb4c4

//AP addresses
#define CC3200_CORE_AP_CSW_ADDR 0x00
#define CC3200_CORE_AP_TRANSFERADDR_ADDR 0x04
#define CC3200_CORE_AP_BANKEDDATA_BANK 0x1
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
#define CC3200_CORE_MEM_DHCSR 0xDF0

//DHCSR bitmasks
#define CC3200_CORE_MEM_DHCSR_C_DEBUGEN (1<<0)
#define CC3200_CORE_MEM_DHCSR_C_HALT (1<<1)
#define CC3200_CORE_MEM_DHCSR_S_HALT (1<<17)
#define CC3200_CORE_MEM_DHCSR_DBGKEY_OFFSET 16
#define CC3200_CORE_MEM_DHCSR_DBGKEY_MASK (0xFFFF<<CC3200_CORE_MEM_DHCSR_DBGKEY_OFFSET)

//misc
#define CC3200_CORE_DBGKEY 0xA05F //"key" to unlock debug halting register writes
#define CC3200_CORE_CSW_SIZE_WORD 0x02 //this setting of SIZE bits in MEM-AP CSW enables 32-bit memory accesses.

//initialize the core access port. Assumes cc3200_jtagdp is initialized already.
int cc3200_core_init(void);

//detect the core access port. Assumes this module is initialized already.
int cc3200_core_detect(void);

//read a register of the access port. regaddr is the combined bank and register address.
int cc3200_core_read_APreg(uint8_t ap, uint8_t regaddr, uint32_t* result);

//write a register of the access port. regaddr is the combined bank and register address.
int cc3200_core_write_APreg(uint8_t ap, uint8_t regaddr, uint32_t value);

//read a memory location from the core system memory space.
int cc3200_core_read_mem_addr(uint32_t addr, uint32_t* result);

//write a memory location from the core system memory space.
int cc3200_core_write_mem_addr(uint32_t addr, uint32_t value);

//halt the core.
int cc3200_core_debug_halt(void);

//enable debug.
int cc3200_core_debug_enable(void);

//get the debug base address.
uint32_t cc3200_core_get_debug_base(void);

#endif
