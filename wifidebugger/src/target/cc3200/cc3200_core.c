/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "cc3200_core.h"
#include "cc3200_jtagdp.h"
#include "common.h"
#include "error.h"
#include "mem_log.h"

//ROM table
struct cc3200_core_romtable_entry{
    uint8_t valid;
    uint32_t address;
};

struct cc3200_core_state_t{
    int initialized;
    int detected;
    uint32_t AP_IDR;
    uint32_t debug_base_addr;
    uint8_t debug_entry_present;
    uint8_t device_enabled;
    uint8_t secure_priv_enabled;
    uint8_t debug_sw_enabled;
    uint8_t has_rom_table;
    uint8_t is_big_endian;
    uint32_t compID;
    uint64_t perID;
    struct cc3200_core_romtable_entry romtable[1023];
};
struct cc3200_core_state_t cc3200_core_state = {
    .initialized = 0,
    .detected = 0,
    .AP_IDR = 0,
    .debug_base_addr = 0,
    .device_enabled = 0,
    .secure_priv_enabled = 0,
    .debug_sw_enabled = 0,
    .has_rom_table = 0,
    .debug_entry_present = 0,
    .is_big_endian = 0,
    .compID = 0,
    .perID = 0
};

int cc3200_core_init(void)
{
    if(cc3200_core_state.initialized) return RET_SUCCESS;

    cc3200_core_state.initialized = 1;
    return RET_SUCCESS;
}

int cc3200_core_detect(void)
{
    cc3200_core_state.detected = 0;
    if(!cc3200_core_state.initialized) RETURN_ERROR(ERROR_UNKNOWN);

    if(cc3200_core_read_APreg(0, CC3200_CORE_AP_IDR_ADDR, &cc3200_core_state.AP_IDR) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_state.AP_IDR != CC3200_CORE_AP_IDR) RETURN_ERROR(ERROR_UNKNOWN);

    uint32_t csw;
    if(cc3200_core_read_APreg(0, CC3200_CORE_AP_CSW_ADDR, &csw) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    cc3200_core_state.device_enabled = (csw & CC3200_CORE_CSW_DEVICEEN) ? 1 : 0;
    cc3200_core_state.secure_priv_enabled = (csw & CC3200_CORE_CSW_SPIDEN) ? 1 : 0;
    cc3200_core_state.debug_sw_enabled = (csw & CC3200_CORE_CSW_DBGSWENABLE) ? 1 : 0;

    uint32_t cfg;
    if(cc3200_core_read_APreg(0, CC3200_CORE_AP_CFG_ADDR, &cfg) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    cc3200_core_state.is_big_endian = (cfg & 1);

    if(!cc3200_core_state.device_enabled) RETURN_ERROR(ERROR_UNKNOWN);

    cc3200_core_state.detected = 1;
    return RET_SUCCESS;
}

int cc3200_core_read_APreg(uint8_t ap, uint8_t regaddr, uint32_t* result)
{
    if(!cc3200_core_state.initialized) RETURN_ERROR(ERROR_UNKNOWN);

    if(!cc3200_jtagdp_selectAPBank(ap, (regaddr>>4) & 0xF) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_jtagdp_APACC_read(regaddr & 0xF, result) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    return RET_SUCCESS;
}

int cc3200_core_write_APreg(uint8_t ap, uint8_t regaddr, uint32_t value)
{
    if(!cc3200_core_state.initialized) RETURN_ERROR(ERROR_UNKNOWN);

    if(!cc3200_jtagdp_selectAPBank(ap, (regaddr>>4) & 0xF) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_jtagdp_APACC_write(regaddr & 0xF, value) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    return RET_SUCCESS;
}

int cc3200_core_read_mem_addr(uint32_t addr, uint32_t* result)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    if(cc3200_core_write_APreg(0,CC3200_CORE_AP_TRANSFERADDR_ADDR,addr) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_read_APreg(0,CC3200_CORE_AP_DATARW_ADDR,result) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    return RET_SUCCESS;
}

int cc3200_core_write_mem_addr(uint32_t addr, uint32_t value)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    if(cc3200_core_write_APreg(0,CC3200_CORE_AP_TRANSFERADDR_ADDR,addr) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_write_APreg(0,CC3200_CORE_AP_DATARW_ADDR,value) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    return RET_SUCCESS;
}

int cc3200_core_debug_halt(void)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    if(cc3200_core_write_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_MEM_DHCSR,
            (CC3200_CORE_DBGKEY << CC3200_CORE_MEM_DHCSR_DBGKEY_OFFSET) |
            (CC3200_CORE_MEM_DHCSR_C_DEBUGEN) |
            CC3200_CORE_MEM_DHCSR_C_HALT) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    uint32_t temp;
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_MEM_DHCSR,
            &temp) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(!(temp & CC3200_CORE_MEM_DHCSR_S_HALT)) RETURN_ERROR(temp);

    return RET_SUCCESS;
}

int cc3200_core_debug_enable(void)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    uint32_t temp_entry;
    uint32_t cur_addr = 0;

    if(cc3200_core_read_APreg(0, CC3200_CORE_AP_BASE_ADDR, &cc3200_core_state.debug_base_addr) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    cc3200_core_state.debug_entry_present = cc3200_core_state.debug_base_addr & 1;
    cc3200_core_state.debug_base_addr &= 0xFFFFF000;

    //TODO BUG: For some reason, the above yields the wrong result (0xff000 instead of 0xE000E000).
    //The Cortex M4 reference manual gives 0xE000E000 as the correct debug base address, and this indeed
    //leads to correct results. Find out why the above (reading the base from the BASE register in MEM-AP)
    //doesn't work!
    cc3200_core_state.debug_base_addr = 0xE000E000;

    //read component and peripheral ID's
    uint32_t compID[4]; //temp
    uint32_t perID[5]; //temp
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_COMPID0,
            &(compID[0])) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_COMPID1,
            &(compID[1])) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_COMPID2,
            &(compID[2])) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_COMPID3,
            &(compID[3])) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_PERID0,
            &(perID[0])) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_PERID1,
            &(perID[1])) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_PERID2,
            &(perID[2])) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_PERID3,
            &(perID[3])) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_PERID4,
            &(perID[4])) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    cc3200_core_state.compID = compID[0] | compID[1]<<8 | compID[2]<<16 | compID[3]<<24;
    cc3200_core_state.perID = perID[0] | perID[1]<<8 | perID[2]<<16 | perID[3]<<24 | ((uint64_t)perID[4])<<32;

    //check the component ID's
    if(cc3200_core_state.compID != CC3200_CORE_COMPID) RETURN_ERROR(cc3200_core_state.compID);

    cur_addr = cc3200_core_state.debug_base_addr;
    cc3200_core_state.has_rom_table = 0;

    //read ROM table
    for(int i=0; i<1023; i++){
        if(cc3200_core_read_mem_addr(cur_addr, &temp_entry) == RET_FAILURE) RETURN_ERROR(cur_addr);
        cc3200_core_state.romtable[i].valid = temp_entry&1;
        if(!cc3200_core_state.romtable[i].valid || (temp_entry == 0)){
            mem_log_add("Number of ROM table entries found:", i);
            break;
        }
        if(i>0) cc3200_core_state.has_rom_table = 1; //have read at least 1 valid entry
        uint32_t minus_offset = (uint32_t)(-1*(temp_entry>>12)); //make positive, unsigned
        cc3200_core_state.romtable[i].address = cc3200_core_state.debug_base_addr - minus_offset;
        cur_addr += 4;
    }

    //unlock debugging using the debug key
    if(cc3200_core_write_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_MEM_DHCSR,
            CC3200_CORE_DBGKEY << CC3200_CORE_MEM_DHCSR_DBGKEY_OFFSET) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_write_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_MEM_DHCSR,
            (CC3200_CORE_DBGKEY << CC3200_CORE_MEM_DHCSR_DBGKEY_OFFSET) |
            CC3200_CORE_MEM_DHCSR_C_DEBUGEN) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    uint32_t temp;
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_MEM_DHCSR,
            &temp) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(!(temp & CC3200_CORE_MEM_DHCSR_C_DEBUGEN)) RETURN_ERROR(temp);

    return RET_SUCCESS;
}

uint32_t cc3200_core_get_debug_base(void)
{
    return cc3200_core_state.debug_base_addr;
}
