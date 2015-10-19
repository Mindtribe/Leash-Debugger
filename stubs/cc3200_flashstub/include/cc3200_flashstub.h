/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

/*
 * This file contains definitions used when Leash Debugger requires access to the target CC3200's flash memory.
 * A flash stub will be loaded into target RAM, after which structures defined here are used to communicate
 * with the stub through shared RAM (accessed locally by the target, and over JTAG by the debug adapter).
 */

#ifndef FLASH_PROTOCOL_H_
#define FLASH_PROTOCOL_H_

enum flash_command_type{
    FD_OPEN = 0,
    FD_CLOSE,
    FD_READ,
    FD_WRITE,
    FD_DELETE,
    FD_CRC
};

#define FLASH_RESPONSE_INITIALIZED 2

#define FLASHSTUB_FILENAME "/cc3200_flashstub.bin"

enum mem_sync_state{
    SYNC_UNINIT = 0,
    SYNC_WAIT,
    SYNC_READY
};

struct flash_command_t{
    unsigned int type;
    unsigned int data_size;
};

struct flash_command_response_t{
    int retval;
};

struct command_file_open_args_t{
    unsigned int AccessModeAndMaxSize;
    unsigned char* pFileName;
    long FileHandle;
};

struct command_file_close_args_t{
    int FileHdl;
};

struct command_file_read_args_t{
    int FileHdl;
    unsigned int Offset;
    unsigned int Len;
    unsigned char* pData;
};

struct command_file_write_args_t{
    int FileHdl;
    unsigned int Offset;
    unsigned int Len;
    unsigned char* pData;
};

struct command_file_delete_args_t{
    unsigned char* pFileName;
};

struct command_get_crc_args_t{
    unsigned char* pFileName;
    unsigned int crc;
};

//The following list consists of absolute memory addresses/sizes used for communication.
//They should never go out of sync with the flash stub's linker script.
#define FLASH_SHARED_BASE_ADDR 0x20024000
#define FLASH_SHARED_SIZE 0x1C000
#define FLASH_CMD_ADDR (FLASH_SHARED_BASE_ADDR)
#define FLASH_RESPONSE_ADDR ((FLASH_CMD_ADDR)+sizeof(struct flash_command_t))
#define FLASH_CMD_SYNC_OBJECT_ADDR ((FLASH_RESPONSE_ADDR)+sizeof(struct flash_command_response_t))
#define FLASH_RESPONSE_SYNC_OBJECT_ADDR ((FLASH_CMD_SYNC_OBJECT_ADDR)+sizeof(unsigned int))
#define FLASH_DATA_ADDR ((FLASH_RESPONSE_SYNC_OBJECT_ADDR)+sizeof(unsigned int))
#define FLASH_DATA_SIZE (1024)

//combines all communication structures that should be 0-initialized
struct flash_comm_region_t{
    struct flash_command_t command;
    struct flash_command_response_t response;
    unsigned int command_sync;
    unsigned int response_sync;
};

#endif
