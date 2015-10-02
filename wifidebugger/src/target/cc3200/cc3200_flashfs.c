/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "cc3200_flashfs.h"
#include "cc3200_flash.h" //in flash stub project
#include "cc3200.h"
#include "cc3200_reg.h"

#include "mem.h"
#include "error.h"
#include "log.h"

//Note: referencing this assembly symbol here prevents the linker from
//optimizing out its section at link-time
extern void* dummy_cc3200_flashstub_binary __asm__("CC3200_FLASHSTUB_BINARY");

//Note: these symbols are placed by the linker script.
extern unsigned char _cc3200_flashstub_binary;
extern unsigned char _ecc3200_flashstub_binary;

#define TARGET_SRAM_ORIGIN 0x20004000
#define FLASH_LOAD_CHUNK_SIZE 128

int cc3200_flashfs_loadstub(void){

    LOG(LOG_IMPORTANT, "[CC3200] Loading CC3200 flash stub (may take long)...");

    unsigned int binary_size = (unsigned int)(&_ecc3200_flashstub_binary - &_cc3200_flashstub_binary)+4;

    unsigned int bytes_left = binary_size;
    unsigned int rel_write_addr = 0;
    int retval;
    while(bytes_left>0){
        LOG(LOG_VERBOSE, "[CC3200] (%d/%d)", rel_write_addr, binary_size);
        unsigned int write_bytes = bytes_left;
        if(write_bytes > FLASH_LOAD_CHUNK_SIZE) { write_bytes = FLASH_LOAD_CHUNK_SIZE; }
        retval = cc3200_mem_block_write(TARGET_SRAM_ORIGIN+rel_write_addr,
                write_bytes,
                (unsigned char*)(&_cc3200_flashstub_binary+rel_write_addr));
        if(retval == RET_FAILURE) RETURN_ERROR(retval);
        bytes_left -= write_bytes;
        rel_write_addr += write_bytes;
    }

    LOG(LOG_VERBOSE, "[CC3200] Starting flash stub execution...");


    retval = cc3200_mem_write(FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_UNINIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_reg_write(CC3200_REG_SP, 0x2000897c); //TODO: make this kosher
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_reg_write(CC3200_REG_PC, 0x200044b1); //TODO: make this kosher
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_continue();
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    LOG(LOG_VERBOSE, "[CC3200] Waiting for flash stub initialization...");

    uint32_t syncstate = SYNC_UNINIT;
    do{
        retval = cc3200_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval);
    }while(syncstate != SYNC_READY);

    LOG(LOG_IMPORTANT, "[CC3200] Flash stub ready.");

    return RET_SUCCESS;
}
