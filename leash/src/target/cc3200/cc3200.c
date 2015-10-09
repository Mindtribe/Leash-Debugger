/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "cc3200.h"

#include <stdio.h>
#include "log.h"

#include "error.h"
#include "jtag_scan.h"
#include "cc3200_icepick.h"
#include "cc3200_core.h"
#include "cc3200_jtagdp.h"

#define CC3200_CORE_DFSR_BKPT (1<<1)

#define CC3200_OPCODE_BKPT 0xBE00

#define CC3200_SEMIHOST_WRITE0 0x04
#define CC3200_SEMIHOST_READ 0x06

struct target_al_interface cc3200_interface = {
    .target_init = &cc3200_init,
    .target_reset = &cc3200_reset,
    .target_halt = &cc3200_halt,
    .target_continue = &cc3200_continue,
    .target_step = &cc3200_step,
    .target_mem_read = &cc3200_mem_read,
    .target_mem_write = &cc3200_mem_write,
    .target_mem_block_read = &cc3200_mem_block_read,
    .target_mem_block_write = &cc3200_mem_block_write,
    .target_get_gdb_reg_string = &cc3200_get_gdb_reg_string,
    .target_put_gdb_reg_string = &cc3200_put_gdb_reg_string,
    .target_set_sw_bkpt = &cc3200_set_sw_bkpt,
    .target_poll_halted = &cc3200_poll_halted,
    .target_handleHalt = &cc3200_handleHalt,
    .target_read_register = &cc3200_reg_read,
    .target_write_register = &cc3200_reg_write,
    .target_querySemiHostOp = &cc3200_querySemiHostOp,
    .target_set_pc = &cc3200_set_pc,
    .target_get_pc = &cc3200_get_pc
};

struct cc3200_state_t{
    char regstring[CC3200_REG_LAST+1];
};
struct cc3200_state_t cc3200_state = {
    .regstring = {0}
};

int cc3200_init(void)
{
    jtag_scan_init();

    //ICEPICK router detection and configuration
    if(cc3200_icepick_init() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_icepick_detect() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_icepick_connect() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_icepick_configure() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    LOG(LOG_VERBOSE, "[CC3200] ICEPICK OK.");

    //warm reset
    if(cc3200_icepick_warm_reset() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    LOG(LOG_VERBOSE, "CC3200 - Reset done.");

    //ARM core debug interface (JTAG-DP) detection
    if(cc3200_jtagdp_init(6, ICEPICK_IR_BYPASS, 1, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_jtagdp_detect() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_jtagdp_clearCSR() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    uint32_t csr;
    if(cc3200_jtagdp_checkCSR(&csr) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    LOG(LOG_VERBOSE, "[CC3200] JTAG-DP OK.");

    //powerup
    if(cc3200_jtagdp_powerUpDebug() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    LOG(LOG_VERBOSE, "[CC3200] Powerup OK.");

    if(cc3200_jtagdp_readAPs() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    LOG(LOG_VERBOSE, "[CC3200] IDCODES OK.");

    //core module
    if(cc3200_core_init() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_core_detect() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    LOG(LOG_VERBOSE, "[CC3200] MEM-AP OK.");

    //enable debug
    if(cc3200_core_debug_enable() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    LOG(LOG_VERBOSE, "[CC3200] Debug Enabled.");

    return RET_SUCCESS;
}

int cc3200_reset(void)
{
    jtag_scan_init();

    //ICEPICK router detection and configuration
    if(cc3200_icepick_init() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_icepick_detect() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_icepick_connect() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_icepick_configure() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    LOG(LOG_VERBOSE, "CC3200 - ICEPICK OK.");

    //warm reset
    if(cc3200_icepick_warm_reset() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    LOG(LOG_VERBOSE, "CC3200 - Reset done.");

    return RET_SUCCESS;
}

int cc3200_halt(void)
{
    return cc3200_core_debug_halt();
}

int cc3200_continue(void)
{
    return cc3200_core_debug_continue();
}

int cc3200_step(void)
{
    return cc3200_core_debug_step();
}

int cc3200_set_sw_bkpt(uint32_t addr, uint8_t len_bytes)
{
    if(len_bytes != 2) {RETURN_ERROR(ERROR_UNKNOWN);} //only support 2-byte breakpoints
    if(addr&1) {RETURN_ERROR(ERROR_UNKNOWN);} //non-aligned

    //get word at that memory location
    uint32_t ori;
    if(cc3200_core_read_mem_addr(addr&0xFFFFFFFC, &ori) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    //modify word
    if(addr&0x02){
        ori &= 0x0000FFFF;
        ori |= (CC3200_OPCODE_BKPT << 16);
    }
    else{
        ori &= 0xFFFF0000;
        ori |= CC3200_OPCODE_BKPT;
    }

    //write back
    if(cc3200_core_write_mem_addr(addr&0xFFFFFFFC, ori) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}

int cc3200_mem_read(uint32_t addr, uint32_t* dst)
{
    return cc3200_core_read_mem_addr(addr, dst);
}

int cc3200_mem_write(uint32_t addr, uint32_t value)
{
    return cc3200_core_write_mem_addr(addr, value);
}

int cc3200_get_gdb_reg_string(char* string)
{
    struct cc3200_reg_list reglst;

    for(uint32_t i=0; i<=(uint32_t)CC3200_REG_LAST; i++){
        if(cc3200_core_read_reg(i, &(((uint32_t *)&reglst)[i])) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    }

    string[8*CC3200_REG_LAST] = 0; //zero-terminate

    uint32_t *val = (uint32_t*)&reglst;
    for(enum cc3200_reg_index i=0; i<CC3200_REG_LAST; i++){
        //bytewise convert the register to hex characters.
        sprintf(&(string[8*i]), "%02X%02X%02X%02X",
                (unsigned int)(0xFF & (val[i]>>0)),
                (unsigned int)(0xFF & (val[i]>>8)),
                (unsigned int)(0xFF & (val[i]>>16)),
                (unsigned int)(0xFF & (val[i]>>24))
        );
    }

    return RET_SUCCESS;
}

int cc3200_put_gdb_reg_string(char* string)
{
    //TODO: string validity tests

    for(int i = 0; string[i] != 0 && i<CC3200_REG_LAST; i++){
        if(string[i*8] == 'x') {continue;} //skip this register
        if(i==CC3200_REG_XPSR) {continue;} //TODO: writing to XPSR seems to cause faults every time. Investigate.

        uint32_t reg = 0; //the register value
        unsigned int reg0, reg8, reg16, reg24;
        sscanf(&(string[i*8]), "%02X%02X%02X%02X",
                &reg0,
                &reg8,
                &reg16,
                &reg24);
        reg = reg0 | (reg8<<8) | (reg16<<16) | (reg24<<24);

        if(cc3200_core_write_reg(i, reg) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    }
    return RET_SUCCESS;
}

int cc3200_mem_block_read(uint32_t addr, uint32_t bytes, uint8_t *dst)
{
    uint32_t data;
    uint8_t *data_bytes = (uint8_t*)&data;

    if((bytes%4 ==0) && (addr%4 ==0) && (bytes>4)){ //word-aligned full-word reads
        if(cc3200_core_pipeline_read_mem_addr(addr, bytes/4, (uint32_t*) dst) == RET_FAILURE){
            {RETURN_ERROR(ERROR_UNKNOWN);}
        }
    }
    else{
        //non-word-aligned or incomplete words

        uint32_t last_read_addr = addr & 0xFFFFFFFC;
        if(cc3200_core_read_mem_addr(last_read_addr, &data) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

        int out_byte = 0;
        for(uint32_t i=0; i<bytes; i++){
            if((addr+i)/4 != last_read_addr/4){ //different word?
                last_read_addr = (addr+i)& 0xFFFFFFFC;
                if(cc3200_core_read_mem_addr(last_read_addr, &data) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
            }
            dst[out_byte++] = data_bytes[(addr+i)%4];
        }
    }


    return RET_SUCCESS;
}

int cc3200_mem_block_write(uint32_t addr, uint32_t bytes, uint8_t *src)
{
    uint32_t data = 0;
    uint32_t bytes_left = bytes;
    uint8_t *data_bytes = (uint8_t*)&data;
    uint8_t *src_bytes = src;

    if((bytes%4 == 0) && (addr%4 == 0) && (bytes>4)){
        return cc3200_core_pipeline_write_mem_addr(addr, bytes/4, (uint32_t*)src);
    }
    else{

        //BELOW IS THE SLOW METHOD FOR BLOCKS THAT ARE (PARTIALLY) UNALIGNED OR NON-WORD-SIZED
        for(uint32_t cur_addr = addr - (addr%4); cur_addr <= (addr+bytes); cur_addr+=4){
            if(bytes_left>=4 && cur_addr >= addr){ //aligned, whole-word write
                for(int i=0; i<4; i++){ data_bytes[i] = src_bytes[i]; } //prepare the word
                if(cc3200_core_write_mem_addr(cur_addr, data) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);} //write the word
                bytes_left -= 4;
                src_bytes = &(src_bytes[4]);
            }
            else{ //non-aligned and/or partial word access - read-modify-write required
                if(cc3200_core_read_mem_addr(cur_addr, &data) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);} //read original
                for(uint32_t i=0; i<4; i++){
                    if(cur_addr+i >= addr && bytes_left > 0){
                        data_bytes[i] = src_bytes[0];
                        bytes_left--;
                        src_bytes = &(src_bytes[1]);
                    }
                }
                if(cc3200_core_write_mem_addr(cur_addr, data) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);} //write modified value
            }
            data = 0;
        }

    }

    return RET_SUCCESS;
}

int cc3200_set_pc(uint32_t addr)
{
    return cc3200_core_set_pc(addr);
}

int cc3200_get_pc(uint32_t* dst)
{
    return cc3200_core_get_pc(dst);
}

int cc3200_poll_halted(uint8_t *result)
{
    return cc3200_core_poll_halted(result);
}

int cc3200_handleHalt(enum stop_reason *reason)
{
    uint32_t dfsr;
    if(cc3200_core_getDFSR(&dfsr) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(dfsr & CC3200_CORE_DFSR_BKPT){
        uint16_t instruction;
        uint32_t pc;
        if(cc3200_get_pc(&pc) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        if(cc3200_mem_block_read(pc, 2, (uint8_t*)&instruction) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

        //double-check that this is a BKPT instruction
        if((instruction & 0xFF00) != CC3200_OPCODE_BKPT) {RETURN_ERROR(ERROR_UNKNOWN);}

        //0xAB is a special BKPT for semi-hosting
        if((instruction & 0x00FF) == 0xAB) {*reason = STOPREASON_SEMIHOSTING;}
        else *reason = STOPREASON_BREAKPOINT;
    }
    else *reason = STOPREASON_INTERRUPT;
    return RET_SUCCESS;
}

int cc3200_reg_read(uint8_t regnum, uint32_t *dst)
{
    return cc3200_core_read_reg((enum cc3200_reg_index) regnum, dst);
}

int cc3200_reg_write(uint8_t regnum, uint32_t value)
{
    return cc3200_core_write_reg((enum cc3200_reg_index) regnum, value);
}

int cc3200_querySemiHostOp(struct semihost_operation *op)
{
    //determine what kind of semihosting operation is required.
    //TODO: expand support.

    uint32_t r0, r1;
    if(cc3200_reg_read(CC3200_REG_0, &r0) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(cc3200_reg_read(CC3200_REG_1, &r1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    switch(r0){
    case CC3200_SEMIHOST_WRITE0: //write a 0-terminated string
        op->opcode = SEMIHOST_WRITECONSOLE;
        //find out the length of the semihosting string (including trailing 0)
        uint32_t len = 0;
        uint8_t c = 1;
        for(uint32_t addr = r1; c != 0 ;len++){
            if(cc3200_mem_block_read(addr++, 1, &c) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        }
        op->param1 = r1; //pointer to the string
        op->param2 = len; //length of the string
        break;
    case CC3200_SEMIHOST_READ: //read characters
        op->opcode = SEMIHOST_READCONSOLE;
        uint32_t args[3];
        if(cc3200_mem_block_read(r1, 12, (uint8_t*)args) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        op->param1 = args[0]; //file descriptor
        op->param2 = args[1]; //buffer pointer
        op->param3 = args[2]; //character count
        break;
    default:
        op->opcode = SEMIHOST_UNKNOWN;
        break;
    }

    return RET_SUCCESS;
}


