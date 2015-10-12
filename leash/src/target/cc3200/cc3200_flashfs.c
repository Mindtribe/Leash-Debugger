/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "cc3200_flashfs.h"
#include "cc3200_flashstub.h" //in flash stub project
#include "cc3200.h"
#include "cc3200_reg.h"

#include "mem.h"
#include "error.h"
#include "log.h"

#include "simplelink.h"
#include "fs.h"

#include "FreeRTOS.h"
#include "task.h"

#define TARGET_SRAM_ORIGIN 0x20004000
#define FLASH_LOAD_CHUNK_SIZE 128

int cc3200_flashfs_loadstub(void)
{
    unsigned int bytes_left;
    unsigned int rel_write_addr = 0;
    int retval;
    unsigned char databuf[FLASH_LOAD_CHUNK_SIZE];
    int filehandle = -1;
    unsigned int stackptaddr;
    unsigned int entryaddr;

    LOG(LOG_IMPORTANT, "[CC3200] Checking flash stub binary @ %s", FLASHSTUB_FILENAME);

    SlFsFileInfo_t stubinfo;
    retval = sl_FsGetInfo((unsigned char*) FLASHSTUB_FILENAME, 0, &stubinfo);
    if(retval < 0) { RETURN_ERROR(retval); }

    LOG(LOG_IMPORTANT, "[CC3200] Flash stub binary found - size %ub.", (unsigned int)stubinfo.FileLen);

    LOG(LOG_IMPORTANT, "[CC3200] Loading flash stub...");

    retval = sl_FsOpen((unsigned char*)FLASHSTUB_FILENAME, FS_MODE_OPEN_READ, NULL, (_i32*)&filehandle);
    if(retval < 0) { RETURN_ERROR(retval); }

    bytes_left = (unsigned int)stubinfo.FileLen;

    while(bytes_left>0){
        unsigned int write_bytes = bytes_left;
        if(write_bytes > FLASH_LOAD_CHUNK_SIZE) { write_bytes = FLASH_LOAD_CHUNK_SIZE; }
        retval = sl_FsRead((_i32)filehandle, (stubinfo.FileLen - bytes_left), (_u8*)databuf, write_bytes);
        if((unsigned int)retval != write_bytes) {
            sl_FsClose((_i32) filehandle, NULL, NULL, 0);
            RETURN_ERROR(retval);
        }
        if(bytes_left == (unsigned int)stubinfo.FileLen){ //first iteration
            //Read stack pointer and entry address from the binary
            if(bytes_left < 8){
                sl_FsClose((_i32) filehandle, NULL, NULL, 0);
                RETURN_ERROR(retval);
            }
            stackptaddr = ((unsigned int*)databuf)[0];
            entryaddr = ((unsigned int*)databuf)[1];
        }
        retval = cc3200_mem_block_write(TARGET_SRAM_ORIGIN+rel_write_addr,
                write_bytes,
                databuf);
        if(retval == RET_FAILURE){
            sl_FsClose((_i32) filehandle, NULL, NULL, 0);
            RETURN_ERROR(retval);
        }
        bytes_left -= write_bytes;
        rel_write_addr += write_bytes;
    }

    retval = sl_FsClose((_i32) filehandle, NULL, NULL, 0);
    if(retval < 0) { RETURN_ERROR(retval); }

    LOG(LOG_VERBOSE, "[CC3200] Stack ptr: 0x%8X Entry: 0x%8X", stackptaddr, entryaddr);

    retval = cc3200_mem_write(FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_UNINIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_reg_write(CC3200_REG_SP, stackptaddr);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_reg_write(CC3200_REG_PC, entryaddr);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    LOG(LOG_VERBOSE, "[CC3200] Starting flash stub execution...");

    retval = cc3200_continue();
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    LOG(LOG_VERBOSE, "[CC3200] Waiting for flash stub initialization...");

    uint32_t syncstate = SYNC_UNINIT;
    do{
        vTaskDelay(1);
        retval = cc3200_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval);
    }while(syncstate != SYNC_READY);

    LOG(LOG_IMPORTANT, "[CC3200] Flash stub ready.");

    return RET_SUCCESS;
}
