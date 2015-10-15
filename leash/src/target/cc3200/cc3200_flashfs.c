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

int cc3200_flashfs_load(char* pFileName)
{
    unsigned int bytes_left;
    unsigned int rel_write_addr = 0;
    int retval;
    unsigned char databuf[FLASH_LOAD_CHUNK_SIZE];
    int filehandle = -1;

    LOG(LOG_IMPORTANT, "[CC3200] Checking file @ %s", pFileName);

    SlFsFileInfo_t stubinfo;
    retval = sl_FsGetInfo((unsigned char*) pFileName, 0, &stubinfo);
    if(retval < 0) { RETURN_ERROR(retval); }

    LOG(LOG_IMPORTANT, "[CC3200] File found - size %ub.", (unsigned int)stubinfo.FileLen);

    LOG(LOG_IMPORTANT, "[CC3200] Loading file...");

    retval = sl_FsOpen((unsigned char*)pFileName, FS_MODE_OPEN_READ, NULL, (_i32*)&filehandle);
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

    return RET_SUCCESS;
}


int cc3200_flashfs_loadstub(void)
{
    int retval;
    unsigned int stackptaddr;
    unsigned int entryaddr;

    LOG(LOG_VERBOSE, "[CC3200] Starting flash stub load...");

    retval = cc3200_flashfs_load(FLASHSTUB_FILENAME);
    if(retval == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    retval = cc3200_mem_read(0x20004000, (uint32_t*)&stackptaddr);
    if(retval == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    retval = cc3200_mem_read(0x20004004, (uint32_t*)&entryaddr);
    if(retval == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

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

    retval = cc3200_mem_write((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_WAIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    LOG(LOG_IMPORTANT, "[CC3200] Flash stub ready.");

    return RET_SUCCESS;
}

int cc3200_flashfs_open(unsigned int AccessModeAndMaxSize, unsigned char* pFileName, long* pFileHandle)
{
    struct flash_command_t cmd;
    struct command_file_open_args_t args;
    struct flash_command_response_t response;
    int retval;

    //memory structure in shared data region:
    //1. command_file_open_args_t struct
    //2. file name input string

    args.AccessModeAndMaxSize = AccessModeAndMaxSize;
    args.pFileName = (unsigned char*)(FLASH_DATA_ADDR + sizeof(struct command_file_open_args_t));
    args.FileHandle = 0;

    cmd.type = FD_OPEN;

    LOG(LOG_VERBOSE, "[CC3200] Opening file '%s'...", (char*) pFileName);

    retval = cc3200_mem_block_write((unsigned int)FLASH_CMD_ADDR, sizeof(struct flash_command_t), (unsigned char*)&cmd);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_mem_block_write((unsigned int)FLASH_DATA_ADDR, sizeof(struct command_file_open_args_t), (unsigned char*)&args);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_mem_block_write((unsigned int)FLASH_DATA_ADDR + sizeof(struct command_file_open_args_t),
            strlen((char*)pFileName)+1, pFileName);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    //execute
    retval = cc3200_mem_write((unsigned int)FLASH_CMD_SYNC_OBJECT_ADDR, SYNC_READY);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    //wait to finish
    uint32_t syncstate = SYNC_UNINIT;
    do{
        vTaskDelay(1);
        retval = cc3200_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval);
    }while(syncstate != SYNC_READY);
    retval = cc3200_mem_write((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_WAIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    //read response
    retval = cc3200_mem_block_read((unsigned int)FLASH_RESPONSE_ADDR, sizeof(struct flash_command_response_t), (unsigned char*)&response);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_mem_block_read((unsigned int)FLASH_DATA_ADDR, sizeof(struct command_file_open_args_t), (unsigned char*)&args);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    *pFileHandle = args.FileHandle;

    if(response.retval < 0){LOG(LOG_VERBOSE, "[CC3200] ...Failed.");}
    else{LOG(LOG_VERBOSE, "[CC3200] ...Success.");}
    return response.retval;
}

int cc3200_flashfs_close(int FileHdl)
{
    struct flash_command_t cmd;
    struct command_file_close_args_t args;
    struct flash_command_response_t response;
    int retval;

    //memory structure in shared data region:
    //1. command_file_close_args_t struct

    args.FileHdl = FileHdl;

    cmd.type = FD_CLOSE;

    LOG(LOG_VERBOSE, "[CC3200] Closing file...");

    retval = cc3200_mem_block_write((unsigned int)FLASH_CMD_ADDR, sizeof(struct flash_command_t), (unsigned char*)&cmd);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_mem_block_write((unsigned int)FLASH_DATA_ADDR, sizeof(struct command_file_close_args_t), (unsigned char*)&args);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    //execute
    retval = cc3200_mem_write((unsigned int)FLASH_CMD_SYNC_OBJECT_ADDR, SYNC_READY);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    //wait to finish
    uint32_t syncstate = SYNC_UNINIT;
    do{
        vTaskDelay(1);
        retval = cc3200_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval);
    }while(syncstate != SYNC_READY);
    retval = cc3200_mem_write((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_WAIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    //read response
    retval = cc3200_mem_block_read((unsigned int)FLASH_RESPONSE_ADDR, sizeof(struct flash_command_response_t), (unsigned char*)&response);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    if(response.retval < 0){LOG(LOG_VERBOSE, "[CC3200] ...Failed.");}
    else{LOG(LOG_VERBOSE, "[CC3200] ...Success.");}
    return response.retval;
}

int cc3200_flashfs_read(int FileHdl, unsigned int Offset, unsigned char* pData, unsigned int Len)
{
    struct flash_command_t cmd;
    struct command_file_read_args_t args;
    struct flash_command_response_t response;
    int retval;

    //memory structure in shared data region:
    //1. command_file_read_args_t struct
    //2. file data read buffer

    args.FileHdl = FileHdl;
    args.Len = Len;
    args.Offset = Offset;
    args.pData = (unsigned char*)(FLASH_DATA_ADDR + sizeof(struct command_file_read_args_t));

    cmd.type = FD_READ;

    retval = cc3200_mem_block_write((unsigned int)FLASH_CMD_ADDR, sizeof(struct flash_command_t), (unsigned char*)&cmd);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_mem_block_write((unsigned int)FLASH_DATA_ADDR, sizeof(struct command_file_read_args_t), (unsigned char*)&args);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    //execute
    retval = cc3200_mem_write((unsigned int)FLASH_CMD_SYNC_OBJECT_ADDR, SYNC_READY);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    //wait to finish
    uint32_t syncstate = SYNC_UNINIT;
    do{
        vTaskDelay(1);
        retval = cc3200_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval);
    }while(syncstate != SYNC_READY);
    retval = cc3200_mem_write((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_WAIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    //read response
    retval = cc3200_mem_block_read((unsigned int)FLASH_RESPONSE_ADDR, sizeof(struct flash_command_response_t), (unsigned char*)&response);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_mem_block_read((unsigned int)FLASH_DATA_ADDR + sizeof(struct command_file_read_args_t),
            Len, pData);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    //GDB tests the "boundary" by reading at the end of the file.
    //SimpleLink regards this as an error, but the correct response for GDB would be 0 (EOF, 0 bytes read).
    if(response.retval == SL_FS_ERR_OFFSET_OUT_OF_RANGE) { response.retval = 0; }

    return response.retval;
}

int cc3200_flashfs_write(int FileHdl, unsigned int Offset, unsigned char* pData, unsigned int Len)
{
    struct flash_command_t cmd;
    struct command_file_write_args_t args;
    struct flash_command_response_t response;
    int retval;

    //memory structure in shared data region:
    //1. command_file_write_args_t struct
    //2. file data write buffer

    args.FileHdl = FileHdl;
    args.Len = Len;
    args.Offset = Offset;
    args.pData = (unsigned char*)(FLASH_DATA_ADDR + sizeof(struct command_file_write_args_t));

    cmd.type = FD_WRITE;

    retval = cc3200_mem_block_write((unsigned int)FLASH_CMD_ADDR, sizeof(struct flash_command_t), (unsigned char*)&cmd);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_mem_block_write((unsigned int)FLASH_DATA_ADDR, sizeof(struct command_file_write_args_t), (unsigned char*)&args);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_mem_block_write((unsigned int)FLASH_DATA_ADDR + sizeof(struct command_file_write_args_t),
            Len, pData);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    //execute
    retval = cc3200_mem_write((unsigned int)FLASH_CMD_SYNC_OBJECT_ADDR, SYNC_READY);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    //wait to finish
    //TODO timeouts!!!
    uint32_t syncstate = SYNC_UNINIT;
    do{
        vTaskDelay(1);
        retval = cc3200_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval);
    }while(syncstate != SYNC_READY);
    retval = cc3200_mem_write((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_WAIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    //read response
    retval = cc3200_mem_block_read((unsigned int)FLASH_RESPONSE_ADDR, sizeof(struct flash_command_response_t), (unsigned char*)&response);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    return response.retval;
}

int cc3200_flashfs_delete(unsigned char* pFileName)
{
    struct flash_command_t cmd;
    struct command_file_delete_args_t args;
    struct flash_command_response_t response;
    int retval;

    //memory structure in shared data region:
    //1. command_file_delete_args_t struct
    //2. file name input string

    args.pFileName = (unsigned char*)(FLASH_DATA_ADDR + sizeof(struct command_file_delete_args_t));

    cmd.type = FD_DELETE;

    LOG(LOG_VERBOSE, "[CC3200] Deleting file '%s'...", (char*) pFileName);

    retval = cc3200_mem_block_write((unsigned int)FLASH_CMD_ADDR, sizeof(struct flash_command_t), (unsigned char*)&cmd);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_mem_block_write((unsigned int)FLASH_DATA_ADDR, sizeof(struct command_file_delete_args_t), (unsigned char*)&args);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    retval = cc3200_mem_block_write((unsigned int)FLASH_DATA_ADDR + sizeof(struct command_file_delete_args_t),
            strlen((char*)pFileName)+1, pFileName);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);
    //execute
    retval = cc3200_mem_write((unsigned int)FLASH_CMD_SYNC_OBJECT_ADDR, SYNC_READY);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    //wait to finish
    uint32_t syncstate = SYNC_UNINIT;
    do{
        vTaskDelay(1);
        retval = cc3200_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval);
    }while(syncstate != SYNC_READY);
    retval = cc3200_mem_write((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_WAIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    //read response
    retval = cc3200_mem_block_read((unsigned int)FLASH_RESPONSE_ADDR, sizeof(struct flash_command_response_t), (unsigned char*)&response);
    if(retval == RET_FAILURE) RETURN_ERROR(retval);

    if(response.retval < 0){LOG(LOG_VERBOSE, "[CC3200] ...Failed.");}
    else{LOG(LOG_VERBOSE, "[CC3200] ...Success.");}
    return response.retval;
}
