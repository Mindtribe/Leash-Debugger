/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "hw_types.h"
#include "hw_ints.h"
#include "interrupt.h"
#include "prcm.h"
#include "rom_map.h"
#include "cc3200_flashstub.h"

#include "simplelink.h"
#include "fs.h"

#include "gpio_if.h"
#include "pinmux.h"

#include "crc32.h"

#ifndef NULL
#define NULL 0
#endif

#define CRC_READ_CHUNK_SIZE 512

extern void (* const g_pfnVectors[])(void);

static volatile struct flash_comm_region_t *flash = (volatile struct flash_comm_region_t *)FLASH_SHARED_BASE_ADDR;
static volatile void* flash_data = ((void*)(FLASH_DATA_ADDR));

static int BoardInit(void);

int main(void)
{
    long fd;

    BoardInit();

    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    //initialize the flash communication region
    for(size_t i=0; i<sizeof(struct flash_comm_region_t); i++){
        ((unsigned char*)flash)[i] = 0;
    }

    //start the SL driver
    int retval = sl_Start(0,0,0);
    if(retval < 0) {return 1;}

    //tell the debugger stub is ready
    flash->response.retval = FLASH_RESPONSE_INITIALIZED;
    flash->response_sync = SYNC_READY;

    for(;;){
        while(flash->response_sync != SYNC_WAIT)
            ;
        while(flash->command_sync != SYNC_READY)
            ;
        flash->command_sync = SYNC_WAIT;

        switch(flash->command.type){
        case FD_OPEN:
            flash->response.retval = sl_FsOpen(
                    ((struct command_file_open_args_t*)flash_data)->pFileName,
                    ((struct command_file_open_args_t*)flash_data)->AccessModeAndMaxSize,
                    NULL,
                    &((struct command_file_open_args_t*)flash_data)->FileHandle);
            break;
        case FD_CLOSE:
            flash->response.retval = sl_FsClose(
                    ((struct command_file_close_args_t*)flash_data)->FileHdl,
                    NULL, NULL, 0);
            break;
        case FD_READ:
            flash->response.retval = sl_FsRead(
                    ((struct command_file_read_args_t*)flash_data)->FileHdl,
                    ((struct command_file_read_args_t*)flash_data)->Offset,
                    ((struct command_file_read_args_t*)flash_data)->pData,
                    ((struct command_file_read_args_t*)flash_data)->Len);
            break;
        case FD_WRITE:
            flash->response.retval = sl_FsWrite(
                    ((struct command_file_write_args_t*)flash_data)->FileHdl,
                    ((struct command_file_write_args_t*)flash_data)->Offset,
                    ((struct command_file_write_args_t*)flash_data)->pData,
                    ((struct command_file_write_args_t*)flash_data)->Len);
            break;
        case FD_DELETE:
            flash->response.retval = sl_FsDel(
                    ((struct command_file_delete_args_t*)flash_data)->pFileName,
                    0);
            break;
        case FD_CRC:
            flash->response.retval = sl_FsOpen(
                    ((struct command_get_crc_args_t*)flash_data)->pFileName,
                    FS_MODE_OPEN_READ,
                    NULL,
                    &fd);
            ((struct command_get_crc_args_t*)flash_data)->crc = 0;
            if(flash->response.retval<0){
                break;
            }
            unsigned char dataBuf[CRC_READ_CHUNK_SIZE];
            unsigned int offset = 0;
            ((struct command_get_crc_args_t*)flash_data)->crc = 0;
            while((flash->response.retval = sl_FsRead(
                    fd,
                    offset,
                    dataBuf,
                    CRC_READ_CHUNK_SIZE)) > 0){
                offset += (unsigned int) flash->response.retval;
                ((struct command_get_crc_args_t*)flash_data)->crc =
                        (unsigned int)crc32(dataBuf, flash->response.retval,
                                ((struct command_get_crc_args_t*)flash_data)->crc);
                if(flash->response.retval < CRC_READ_CHUNK_SIZE){
                    break;
                }
            }
            if(flash->response.retval<0){
                sl_FsClose(fd, NULL, NULL, 0);
                break;
            }
            flash->response.retval = sl_FsClose(fd, NULL, NULL, 0);
            break;
        default:
            break;
        }

        flash->response_sync = SYNC_READY;
    }

    return 1;
}

static int BoardInit(void)
{
    //init vector table
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);
    PRCMCC3200MCUInit();

    PinMuxConfig();
    GPIO_IF_LedConfigure(LED1|LED2|LED3);

    return 1;
}
