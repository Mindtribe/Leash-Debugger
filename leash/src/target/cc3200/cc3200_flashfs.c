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

static const char* cc3200_flashfs_log_prefix = "[CC3200] ";

int cc3200_flashfs_load(char* pFileName)
{
    unsigned int bytes_left;
    unsigned int rel_write_addr = 0;
    int retval;
    unsigned char databuf[FLASH_LOAD_CHUNK_SIZE];
    int filehandle = -1;

    LOG(LOG_IMPORTANT, "%sChecking file @ %s",cc3200_flashfs_log_prefix , pFileName);

    SlFsFileInfo_t stubinfo;
    retval = sl_FsGetInfo((unsigned char*) pFileName, 0, &stubinfo);
    if(retval < 0) { RETURN_ERROR(retval, "File info fail"); }

    LOG(LOG_IMPORTANT, "%sFile found - size %ub.",cc3200_flashfs_log_prefix , (unsigned int)stubinfo.FileLen);

    LOG(LOG_IMPORTANT, "%sLoading file...",cc3200_flashfs_log_prefix );

    retval = sl_FsOpen((unsigned char*)pFileName, FS_MODE_OPEN_READ, NULL, (_i32*)&filehandle);
    if(retval < 0) { RETURN_ERROR(retval, "File open fail"); }

    bytes_left = (unsigned int)stubinfo.FileLen;

    while(bytes_left>0){
        unsigned int write_bytes = bytes_left;
        if(write_bytes > FLASH_LOAD_CHUNK_SIZE) { write_bytes = FLASH_LOAD_CHUNK_SIZE; }
        retval = sl_FsRead((_i32)filehandle, (stubinfo.FileLen - bytes_left), (_u8*)databuf, write_bytes);
        if((unsigned int)retval != write_bytes) {
            sl_FsClose((_i32) filehandle, NULL, NULL, 0);
            RETURN_ERROR(retval, "File read fail");
        }
        retval = cc3200_interface.target_mem_block_write(TARGET_SRAM_ORIGIN+rel_write_addr,
                write_bytes,
                databuf);
        if(retval == RET_FAILURE){
            sl_FsClose((_i32) filehandle, NULL, NULL, 0);
            RETURN_ERROR(retval, "Mem write fail");
        }
        bytes_left -= write_bytes;
        rel_write_addr += write_bytes;
    }

    retval = sl_FsClose((_i32) filehandle, NULL, NULL, 0);
    if(retval < 0) { RETURN_ERROR(retval, "File close fail"); }

    return RET_SUCCESS;
}


int cc3200_flashfs_loadstub(void)
{
    int retval;
    unsigned int stackptaddr;
    unsigned int entryaddr;

    LOG(LOG_VERBOSE, "%sSearching flash stub...",cc3200_flashfs_log_prefix );

    SlFsFileInfo_t stubinfo;
    int i;
    for(i=0; i<FLASHSTUB_NUM_NAMES; i++){
        retval = sl_FsGetInfo((unsigned char*) FLASHSTUB_FILENAMES[i], 0, &stubinfo);
        if(retval >= 0) { break; }
    }
    if(retval<0){ RETURN_ERROR(ERROR_UNKNOWN, "%sNot found!"); }

    LOG(LOG_VERBOSE, "%sWill load '%s'.",cc3200_flashfs_log_prefix , FLASHSTUB_FILENAMES[i]);

    retval = cc3200_flashfs_load((char*)FLASHSTUB_FILENAMES[i]);
    if(retval == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Stub load fail");}

    retval = cc3200_interface.target_mem_read(0x20004000, (uint32_t*)&stackptaddr);
    if(retval == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Mem read fail");}
    retval = cc3200_interface.target_mem_read(0x20004004, (uint32_t*)&entryaddr);
    if(retval == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Mem read fail");}

    LOG(LOG_VERBOSE, "%sStack ptr: 0x%8X Entry: 0x%8X",cc3200_flashfs_log_prefix , stackptaddr, entryaddr);

    retval = cc3200_interface.target_mem_write(FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_UNINIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    retval = cc3200_interface.target_write_register(CC3200_REG_SP, stackptaddr);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    retval = cc3200_interface.target_write_register(CC3200_REG_PC, entryaddr);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");

    LOG(LOG_VERBOSE, "%sStarting flash stub execution...",cc3200_flashfs_log_prefix );

    retval = cc3200_interface.target_continue();
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Continue fail");

    LOG(LOG_VERBOSE, "%sWaiting for flash stub initialization...",cc3200_flashfs_log_prefix );

    uint32_t syncstate = SYNC_UNINIT;
    do{
        vTaskDelay(1);
        retval = cc3200_interface.target_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");
    }while(syncstate != SYNC_READY);

    retval = cc3200_interface.target_mem_write((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_WAIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");

    LOG(LOG_IMPORTANT, "%sFlash stub ready.",cc3200_flashfs_log_prefix );

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

    LOG(LOG_VERBOSE, "%sOpening file '%s'...",cc3200_flashfs_log_prefix , (char*) pFileName);

    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_CMD_ADDR, sizeof(struct flash_command_t), (unsigned char*)&cmd);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_DATA_ADDR, sizeof(struct command_file_open_args_t), (unsigned char*)&args);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_DATA_ADDR + sizeof(struct command_file_open_args_t),
            strlen((char*)pFileName)+1, pFileName);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    //execute
    retval = cc3200_interface.target_mem_write((unsigned int)FLASH_CMD_SYNC_OBJECT_ADDR, SYNC_READY);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");

    //wait to finish
    uint32_t syncstate = SYNC_UNINIT;
    do{
        vTaskDelay(1);
        retval = cc3200_interface.target_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");
    }while(syncstate != SYNC_READY);
    retval = cc3200_interface.target_mem_write((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_WAIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");

    //read response
    retval = cc3200_interface.target_mem_block_read((unsigned int)FLASH_RESPONSE_ADDR, sizeof(struct flash_command_response_t), (unsigned char*)&response);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");
    retval = cc3200_interface.target_mem_block_read((unsigned int)FLASH_DATA_ADDR, sizeof(struct command_file_open_args_t), (unsigned char*)&args);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");

    *pFileHandle = args.FileHandle;

    if(response.retval < 0){LOG(LOG_VERBOSE, "%s...Failed.",cc3200_flashfs_log_prefix );}
    else{LOG(LOG_VERBOSE, "%s...Success.",cc3200_flashfs_log_prefix );}
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

    LOG(LOG_VERBOSE, "%sClosing file...",cc3200_flashfs_log_prefix );

    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_CMD_ADDR, sizeof(struct flash_command_t), (unsigned char*)&cmd);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_DATA_ADDR, sizeof(struct command_file_close_args_t), (unsigned char*)&args);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    //execute
    retval = cc3200_interface.target_mem_write((unsigned int)FLASH_CMD_SYNC_OBJECT_ADDR, SYNC_READY);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");

    //wait to finish
    uint32_t syncstate = SYNC_UNINIT;
    do{
        vTaskDelay(1);
        retval = cc3200_interface.target_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");
    }while(syncstate != SYNC_READY);
    retval = cc3200_interface.target_mem_write((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_WAIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");

    //read response
    retval = cc3200_interface.target_mem_block_read((unsigned int)FLASH_RESPONSE_ADDR, sizeof(struct flash_command_response_t), (unsigned char*)&response);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");

    if(response.retval < 0){LOG(LOG_VERBOSE, "%s...Failed.",cc3200_flashfs_log_prefix );}
    else{LOG(LOG_VERBOSE, "%s...Success.",cc3200_flashfs_log_prefix );}
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

    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_CMD_ADDR, sizeof(struct flash_command_t), (unsigned char*)&cmd);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_DATA_ADDR, sizeof(struct command_file_read_args_t), (unsigned char*)&args);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    //execute
    retval = cc3200_interface.target_mem_write((unsigned int)FLASH_CMD_SYNC_OBJECT_ADDR, SYNC_READY);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");

    //wait to finish
    uint32_t syncstate = SYNC_UNINIT;
    do{
        vTaskDelay(1);
        retval = cc3200_interface.target_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");
    }while(syncstate != SYNC_READY);
    retval = cc3200_interface.target_mem_write((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_WAIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");

    //read response
    retval = cc3200_interface.target_mem_block_read((unsigned int)FLASH_RESPONSE_ADDR, sizeof(struct flash_command_response_t), (unsigned char*)&response);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");
    retval = cc3200_interface.target_mem_block_read((unsigned int)FLASH_DATA_ADDR + sizeof(struct command_file_read_args_t),
            Len, pData);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");

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

    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_CMD_ADDR, sizeof(struct flash_command_t), (unsigned char*)&cmd);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_DATA_ADDR, sizeof(struct command_file_write_args_t), (unsigned char*)&args);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_DATA_ADDR + sizeof(struct command_file_write_args_t),
            Len, pData);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    //execute
    retval = cc3200_interface.target_mem_write((unsigned int)FLASH_CMD_SYNC_OBJECT_ADDR, SYNC_READY);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");

    //wait to finish
    //TODO timeouts!!!
    uint32_t syncstate = SYNC_UNINIT;
    do{
        vTaskDelay(1);
        retval = cc3200_interface.target_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");
    }while(syncstate != SYNC_READY);
    retval = cc3200_interface.target_mem_write((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_WAIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");

    //read response
    retval = cc3200_interface.target_mem_block_read((unsigned int)FLASH_RESPONSE_ADDR, sizeof(struct flash_command_response_t), (unsigned char*)&response);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");

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

    LOG(LOG_VERBOSE, "%sDeleting file '%s'...",cc3200_flashfs_log_prefix , (char*) pFileName);

    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_CMD_ADDR, sizeof(struct flash_command_t), (unsigned char*)&cmd);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_DATA_ADDR, sizeof(struct command_file_delete_args_t), (unsigned char*)&args);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    retval = cc3200_interface.target_mem_block_write((unsigned int)FLASH_DATA_ADDR + sizeof(struct command_file_delete_args_t),
            strlen((char*)pFileName)+1, pFileName);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");
    //execute
    retval = cc3200_interface.target_mem_write((unsigned int)FLASH_CMD_SYNC_OBJECT_ADDR, SYNC_READY);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");

    //wait to finish
    uint32_t syncstate = SYNC_UNINIT;
    do{
        vTaskDelay(1);
        retval = cc3200_interface.target_mem_read((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, &syncstate);
        if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");
    }while(syncstate != SYNC_READY);
    retval = cc3200_interface.target_mem_write((unsigned int)FLASH_RESPONSE_SYNC_OBJECT_ADDR, SYNC_WAIT);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem write fail");

    //read response
    retval = cc3200_interface.target_mem_block_read((unsigned int)FLASH_RESPONSE_ADDR, sizeof(struct flash_command_response_t), (unsigned char*)&response);
    if(retval == RET_FAILURE) RETURN_ERROR(retval, "Mem read fail");

    if(response.retval < 0){LOG(LOG_VERBOSE, "%s...Failed.",cc3200_flashfs_log_prefix );}
    else{LOG(LOG_VERBOSE, "%s...Success.",cc3200_flashfs_log_prefix );}
    return response.retval;
}
