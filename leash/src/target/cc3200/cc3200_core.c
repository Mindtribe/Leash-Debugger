/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "cc3200_core.h"

#include "log.h"
#include "cc3200_jtagdp.h"
#include "error.h"

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
#define CC3200_CORE_MEM_AIRCR 0xD0C
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

//DEMCR bitmasks
#define CC3200_CORE_MEM_DEMCR_VC_CORERESET (1)

//AIRCR bitmasks
#define CC3200_CORE_MEM_AIRCR_VECTRESET (1)
#define CC3200_CORE_MEM_AIRCR_KEY_OFFSET 16
#define CC3200_CORE_MEM_AIRCR_KEY_MASK (0xFFFF<<CC3200_CORE_MEM_AIRCR_KEY_MASK)

//DCRSR bitmasks
#define CC3200_CORE_MEM_DCRSR_REGWNR (1<<16)
#define CC3200_CORE_MEM_REGSEL_MASK 0x1F

//DFSR bitmasks
#define CC3200_CORE_DFSR_BKPT (1<<1)

//misc

//"keys" to unlock certain core register writes
#define CC3200_CORE_KEY_DFSR 0xA05F //debug control
#define CC3200_CORE_KEY_AIRCR 0x05FA //reset control

#define CC3200_CORE_CSW_SIZE_WORD 0x02 //this setting of SIZE bits in MEM-AP CSW enables 32-bit memory accesses.

//ROM table
struct cc3200_core_romtable_entry{
    uint8_t valid;
    uint32_t address;
};

struct cc3200_core_state_t{
    int initialized;
    int detected;

    //misc properties
    uint32_t AP_IDR;
    uint32_t debug_base_addr;
    uint8_t debug_entry_present;
    uint8_t device_enabled;
    uint8_t secure_priv_enabled;
    uint8_t debug_sw_enabled;
    uint8_t access_size;
    uint8_t is_big_endian;
    uint32_t system_memory_present;

    //Identification
    uint32_t compID;
    uint64_t perID;

    //ROM table
    uint8_t has_rom_table;
    uint32_t rom_table_size;
    struct cc3200_core_romtable_entry romtable[1023];

    //debug features (have entries in ROM table)
    uint8_t has_scs;
    uint8_t has_dwt;
    uint8_t has_fpb;
    uint8_t has_itm;
    uint8_t has_tpiu;
    uint8_t has_etm;
    uint32_t scs_addr;
    uint32_t dwt_addr;
    uint32_t fpb_addr;
    uint32_t itm_addr;
    uint32_t tpiu_addr;
    uint32_t etm_addr;

    //debug status
    uint8_t halted;
};

//disable GCC warning - braces bug 53119 in GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
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
    .rom_table_size = 0,
    .system_memory_present = 0,
    .access_size = 0,
    .perID = 0,
    .has_scs = 0,
    .has_dwt = 0,
    .has_fpb = 0,
    .has_itm = 0,
    .has_tpiu = 0,
    .has_etm = 0,
    .scs_addr = 0,
    .dwt_addr = 0,
    .fpb_addr = 0,
    .itm_addr = 0,
    .tpiu_addr = 0,
    .etm_addr = 0,
    .romtable = {0},
    .halted = 0
};
#pragma GCC diagnostic pop

int cc3200_core_deinit(void)
{
    cc3200_core_state.initialized = 0;
    return RET_SUCCESS;
}

int cc3200_core_init(void)
{
    if(cc3200_core_state.initialized) {return RET_SUCCESS;}

    cc3200_core_state.initialized = 1;
    return RET_SUCCESS;
}

int cc3200_core_detect(void)
{
    cc3200_core_state.detected = 0;
    if(!cc3200_core_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(cc3200_core_read_APreg(0, CC3200_CORE_AP_IDR_ADDR, &cc3200_core_state.AP_IDR, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_state.AP_IDR != CC3200_CORE_AP_IDR) {RETURN_ERROR(ERROR_UNKNOWN);}

    uint32_t csw;
    if(cc3200_core_read_APreg(0, CC3200_CORE_AP_CSW_ADDR, &csw, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(cc3200_core_write_APreg(0, CC3200_CORE_AP_CSW_ADDR,
            (csw & ~(CC3200_CORE_CSW_SIZE_MASK)) | CC3200_CORE_CSW_SIZE_WORD //word-size accesses
            | (CC3200_CORE_CSW_ADDRINC_SINGLE) //auto-increment address (for pipelined accesses)
            , 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(cc3200_core_read_APreg(0, CC3200_CORE_AP_CSW_ADDR, &csw, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    cc3200_core_state.device_enabled = (csw & CC3200_CORE_CSW_DEVICEEN) ? 1 : 0;
    cc3200_core_state.secure_priv_enabled = (csw & CC3200_CORE_CSW_SPIDEN) ? 1 : 0;
    cc3200_core_state.debug_sw_enabled = (csw & CC3200_CORE_CSW_DBGSWENABLE) ? 1 : 0;
    cc3200_core_state.access_size = (csw & CC3200_CORE_CSW_SIZE_MASK);

    uint32_t cfg;
    if(cc3200_core_read_APreg(0, CC3200_CORE_AP_CFG_ADDR, &cfg, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    cc3200_core_state.is_big_endian = (cfg & 1);

    if(!cc3200_core_state.device_enabled) {RETURN_ERROR(ERROR_UNKNOWN);}

    cc3200_core_state.detected = 1;
    return RET_SUCCESS;
}

int cc3200_core_read_APreg(uint8_t ap, uint8_t regaddr, uint32_t* result, uint8_t check_response)
{
    if(!cc3200_core_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(cc3200_jtagdp_selectAPBank(ap, (regaddr>>4) & 0xF) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_jtagdp_APACC_read(regaddr & 0xF, result, check_response) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}

int cc3200_core_write_APreg(uint8_t ap, uint8_t regaddr, uint32_t value, uint8_t check_response)
{
    if(!cc3200_core_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(cc3200_jtagdp_selectAPBank(ap, (regaddr>>4) & 0xF) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_jtagdp_APACC_write(regaddr & 0xF, value, check_response) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}

int cc3200_core_pipeline_write_APreg(uint8_t ap, uint8_t regaddr, uint32_t len, uint32_t *values)
{
    if(!cc3200_core_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(cc3200_jtagdp_selectAPBank(ap, (regaddr>>4) & 0xF) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_jtagdp_APACC_pipeline_write(regaddr & 0xF, len, values, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}

int cc3200_core_pipeline_read_APreg(uint8_t ap, uint8_t regaddr, uint32_t len, uint32_t *dst)
{
    if(!cc3200_core_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(cc3200_jtagdp_selectAPBank(ap, (regaddr>>4) & 0xF) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_jtagdp_APACC_pipeline_read(regaddr & 0xF, len, dst) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}

int cc3200_core_read_mem_addr(uint32_t addr, uint32_t* result)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(cc3200_core_write_APreg(0,CC3200_CORE_AP_TRANSFERADDR_ADDR,addr & 0xFFFFFFF0, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_read_APreg(0,CC3200_CORE_AP_BD0_ADDR + (addr&0xC),result, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}

int cc3200_core_pipeline_write_mem_addr(uint32_t addr, uint32_t len, uint32_t* values)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}

    uint32_t next_kb_boundary = (addr&0xFFFFFC00)+0x400;
    uint32_t chunk_words = (next_kb_boundary - addr)/4;
    uint32_t cur_addr = addr;
    uint32_t *cur_val = values;
    uint32_t words_left = len;
    if(words_left<chunk_words) { chunk_words = words_left; }

    while(cur_addr < (addr+len*4)){
        if(cc3200_core_write_APreg(0,CC3200_CORE_AP_TRANSFERADDR_ADDR,cur_addr, 0) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        if(cc3200_core_pipeline_write_APreg(0,CC3200_CORE_AP_DATARW_ADDR,chunk_words,cur_val) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        cur_addr += chunk_words*4;
        next_kb_boundary += 0x400;
        words_left -= chunk_words;
        cur_val += chunk_words;
        chunk_words = (next_kb_boundary - cur_addr)/4;
        if(words_left<chunk_words) { chunk_words = words_left; }
    }

    return RET_SUCCESS;
}

int cc3200_core_pipeline_read_mem_addr(uint32_t addr, uint32_t len, uint32_t *dst)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}

    uint32_t next_kb_boundary = (addr&0xFFFFFC00)+0x400;
    uint32_t chunk_words = (next_kb_boundary - addr)/4;
    uint32_t cur_addr = addr;
    uint32_t *cur_dst = dst;
    uint32_t words_left = len;
    if(words_left<chunk_words) { chunk_words = words_left; }

    while(cur_addr < (addr+len*4)){
        if(cc3200_core_write_APreg(0,CC3200_CORE_AP_TRANSFERADDR_ADDR,cur_addr, 0) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        if(cc3200_core_pipeline_read_APreg(0,CC3200_CORE_AP_DATARW_ADDR,chunk_words,cur_dst) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        cur_addr += chunk_words*4;
        next_kb_boundary += 0x400;
        words_left -= chunk_words;
        cur_dst += chunk_words;
        chunk_words = (next_kb_boundary - cur_addr)/4;
        if(words_left<chunk_words) { chunk_words = words_left; }
    }

    return RET_SUCCESS;
}

int cc3200_core_write_mem_addr(uint32_t addr, uint32_t value)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(cc3200_core_write_APreg(0,CC3200_CORE_AP_TRANSFERADDR_ADDR,addr & 0xFFFFFFF0, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_write_APreg(0,CC3200_CORE_AP_BD0_ADDR + (addr&0xC),value, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}

int cc3200_core_debug_halt(void)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(cc3200_core_write_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DHCSR,
            (CC3200_CORE_KEY_DFSR << CC3200_CORE_MEM_DHCSR_DBGKEY_OFFSET) |
            (CC3200_CORE_MEM_DHCSR_C_DEBUGEN) |
            CC3200_CORE_MEM_DHCSR_C_HALT) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    uint32_t temp;
    if(cc3200_core_read_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DHCSR,
            &temp) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(!(temp & CC3200_CORE_MEM_DHCSR_S_HALT)) {RETURN_ERROR(temp);}

    cc3200_core_state.halted = 1;

    return RET_SUCCESS;
}

int cc3200_core_debug_reset_halt(void)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected || !cc3200_core_state.halted) {RETURN_ERROR(ERROR_UNKNOWN);}

    uint32_t demcr;
    if(cc3200_core_read_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DEMCR,
            &demcr) == RET_FAILURE) {
        RETURN_ERROR(ERROR_UNKNOWN);
    }
    demcr |= CC3200_CORE_MEM_DEMCR_VC_CORERESET; //set reset debug trap flag
    if(cc3200_core_write_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DEMCR,
            demcr) == RET_FAILURE) {
        RETURN_ERROR(ERROR_UNKNOWN);
    }

    //request a local core reset
    if(cc3200_core_write_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_AIRCR,
            (CC3200_CORE_KEY_AIRCR << CC3200_CORE_MEM_AIRCR_KEY_OFFSET) |
            (CC3200_CORE_MEM_AIRCR_VECTRESET)) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    uint8_t halted = 0;
    while(!halted){
        if(cc3200_core_poll_halted(&halted) == RET_FAILURE){
            RETURN_ERROR(ERROR_UNKNOWN);
        }
    }

    return RET_SUCCESS;
}

int cc3200_core_debug_step(void)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(!cc3200_core_state.halted) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(cc3200_core_write_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DHCSR,
            (CC3200_CORE_KEY_DFSR << CC3200_CORE_MEM_DHCSR_DBGKEY_OFFSET) |
            (CC3200_CORE_MEM_DHCSR_C_DEBUGEN) |
            CC3200_CORE_MEM_DHCSR_C_STEP) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    uint32_t temp;
    if(cc3200_core_read_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DHCSR,
            &temp) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(!(temp & CC3200_CORE_MEM_DHCSR_S_HALT)) {RETURN_ERROR(temp);}

    cc3200_core_state.halted = 1;

    return RET_SUCCESS;
}

int cc3200_core_debug_continue(void)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(!cc3200_core_state.halted) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(cc3200_core_write_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DHCSR,
            (CC3200_CORE_KEY_DFSR << CC3200_CORE_MEM_DHCSR_DBGKEY_OFFSET) |
            (CC3200_CORE_MEM_DHCSR_C_DEBUGEN)) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    uint32_t temp;
    if(cc3200_core_read_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DHCSR,
            &temp) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    cc3200_core_state.halted = 0;

    return RET_SUCCESS;
}

int cc3200_core_debug_enable(void)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}

    //get debug space base address
    if(cc3200_core_read_APreg(0, CC3200_CORE_AP_BASE_ADDR, &cc3200_core_state.debug_base_addr, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    cc3200_core_state.debug_entry_present = cc3200_core_state.debug_base_addr & 1;
    cc3200_core_state.debug_base_addr &= 0xFFFFF000;

    //read component and peripheral ID's
    uint32_t compID[4]; //temp
    uint32_t perID[5]; //temp
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_COMPID0,
            &(compID[0])) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_COMPID1,
            &(compID[1])) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_COMPID2,
            &(compID[2])) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_COMPID3,
            &(compID[3])) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_PERID0,
            &(perID[0])) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_PERID1,
            &(perID[1])) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_PERID2,
            &(perID[2])) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_PERID3,
            &(perID[3])) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_PERID4,
            &(perID[4])) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    cc3200_core_state.compID = compID[0] | compID[1]<<8 | compID[2]<<16 | compID[3]<<24;
    cc3200_core_state.perID = perID[0] | perID[1]<<8 | perID[2]<<16 | perID[3]<<24 | ((uint64_t)perID[4])<<32;



    //check the component ID's
    if(cc3200_core_state.compID != CC3200_CORE_COMPID) {RETURN_ERROR(cc3200_core_state.compID);}
    if(cc3200_core_state.perID != CC3200_CORE_PERID) {RETURN_ERROR(cc3200_core_state.perID);}


    //check whether system memory is accessible.
    if(cc3200_core_read_mem_addr(cc3200_core_state.debug_base_addr + CC3200_CORE_BASEOFFSET_MEMTYPE,
            &cc3200_core_state.system_memory_present) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}



    //read ROM table
    uint32_t temp_entry;
    uint32_t cur_addr = 0;

    cur_addr = cc3200_core_state.debug_base_addr;
    cc3200_core_state.has_rom_table = 0;

    for(int i=0; i<1023; i++){
        if(cc3200_core_read_mem_addr(cur_addr, &temp_entry) == RET_FAILURE) {RETURN_ERROR(cur_addr);}
        cc3200_core_state.romtable[i].valid = temp_entry&1;
        if(!cc3200_core_state.romtable[i].valid || (temp_entry == 0)){
            LOG(LOG_VERBOSE, "[CC3200 CORE] Number of ROM table entries found: %d", i);
            break;
        }
        if(i>0) {cc3200_core_state.has_rom_table = 1; }//have read at least 1 valid entry
        uint32_t minus_offset = (uint32_t)(-1*((int)(temp_entry&0xFFFFF000))); //make positive, unsigned
        cc3200_core_state.romtable[i].address = cc3200_core_state.debug_base_addr - minus_offset;

        cur_addr += 4;
    }

    //some checks
    if(!cc3200_core_state.has_rom_table) {RETURN_ERROR(ERROR_UNKNOWN);} //cc3200 should have a ROM table.
    if((cc3200_core_state.has_scs = cc3200_core_state.romtable[0].valid) != 0) {cc3200_core_state.scs_addr = cc3200_core_state.romtable[0].address;   }
    if((cc3200_core_state.has_dwt = cc3200_core_state.romtable[1].valid) != 0) {cc3200_core_state.dwt_addr = cc3200_core_state.romtable[1].address;   }
    if((cc3200_core_state.has_fpb = cc3200_core_state.romtable[2].valid) != 0) {cc3200_core_state.fpb_addr = cc3200_core_state.romtable[2].address;   }
    if((cc3200_core_state.has_itm = cc3200_core_state.romtable[3].valid) != 0) {cc3200_core_state.itm_addr = cc3200_core_state.romtable[3].address;   }
    if((cc3200_core_state.has_tpiu = cc3200_core_state.romtable[4].valid) != 0){cc3200_core_state.tpiu_addr = cc3200_core_state.romtable[4].address; }
    if((cc3200_core_state.has_etm = cc3200_core_state.romtable[5].valid) != 0) {cc3200_core_state.etm_addr = cc3200_core_state.romtable[5].address;   }


    //unlock debugging using the debug key
    if(cc3200_core_write_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DHCSR,
            CC3200_CORE_KEY_DFSR << CC3200_CORE_MEM_DHCSR_DBGKEY_OFFSET) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_write_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DHCSR,
            (CC3200_CORE_KEY_DFSR << CC3200_CORE_MEM_DHCSR_DBGKEY_OFFSET) |
            CC3200_CORE_MEM_DHCSR_C_DEBUGEN) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    uint32_t temp;
    if(cc3200_core_read_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DHCSR,
            &temp) == RET_FAILURE){
        RETURN_ERROR(ERROR_UNKNOWN);
    }
    if(!(temp & CC3200_CORE_MEM_DHCSR_C_DEBUGEN)){
        RETURN_ERROR(temp);
    }

    return RET_SUCCESS;
}

uint32_t cc3200_core_get_debug_base(void)
{
    return cc3200_core_state.debug_base_addr;
}

int cc3200_core_read_reg(enum cc3200_reg_index reg, uint32_t* dst)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(!cc3200_core_state.halted) {RETURN_ERROR(ERROR_UNKNOWN);} //must be halted to read a register

    uint32_t regnum = (uint32_t) reg;

    //select the register
    if(cc3200_core_write_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DCRSR,
            regnum & CC3200_CORE_MEM_REGSEL_MASK) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    //wait for the transaction to finish
    uint32_t dhcsr = 0;
    int i;
    for(i=0; i<100 && !(dhcsr & CC3200_CORE_MEM_DHCSR_S_REGRDY); i++){
        if(cc3200_core_read_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DHCSR, &dhcsr)
                == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    }
    if(i>=100) {RETURN_ERROR(ERROR_UNKNOWN);}

    //read the result out
    if(cc3200_core_read_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DCRDR, dst)
            == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}

int cc3200_core_write_reg(enum cc3200_reg_index reg, uint32_t value)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(!cc3200_core_state.halted) {RETURN_ERROR(ERROR_UNKNOWN);} //must be halted to access a register

    uint32_t regnum = (uint32_t) reg;

    //prepare the data
    if(cc3200_core_write_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DCRDR, value)
            == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    //select the register and initialize the transaction
    if(cc3200_core_write_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DCRSR,
            (regnum & CC3200_CORE_MEM_REGSEL_MASK) | CC3200_CORE_MEM_DCRSR_REGWNR) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    //wait for the transaction to finish
    uint32_t dhcsr;
    for(int i=0; i<100; i++){
        if(cc3200_core_read_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DHCSR, &dhcsr)
                == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        if((dhcsr & CC3200_CORE_MEM_DHCSR_S_REGRDY)) {return RET_SUCCESS;}
    }

    return RET_FAILURE;
}

int cc3200_core_set_pc(uint32_t addr)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(!cc3200_core_state.halted) {RETURN_ERROR(ERROR_UNKNOWN);}

    return cc3200_core_write_reg(CC3200_REG_PC, addr);
}

int cc3200_core_get_pc(uint32_t *dst)
{
    if(!cc3200_core_state.initialized || !cc3200_core_state.detected) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(!cc3200_core_state.halted) {RETURN_ERROR(ERROR_UNKNOWN);}

    return cc3200_core_read_reg(CC3200_REG_PC, dst);
}

int cc3200_core_poll_halted(uint8_t *result)
{
    uint32_t temp;
    if(cc3200_core_read_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DHCSR,
            &temp) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(temp & CC3200_CORE_MEM_DHCSR_S_HALT){
        *result = cc3200_core_state.halted = 1;
    }
    else{
        *result = cc3200_core_state.halted = 0;
    }

    return RET_SUCCESS;
}

int cc3200_core_getDFSR(uint32_t *result)
{
    if(cc3200_core_read_mem_addr(cc3200_core_state.scs_addr + CC3200_CORE_MEM_DFSR,
            result) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    return RET_SUCCESS;
}
