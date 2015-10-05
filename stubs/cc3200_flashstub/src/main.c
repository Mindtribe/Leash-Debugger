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
#include "args.h"
#include "cc3200_flash.h"

#include "simplelink.h"
#include "fs.h"

#include "gpio_if.h"
#include "pinmux.h"

#ifndef NULL
#define NULL 0
#endif

extern void (* const g_pfnVectors[])(void);

static volatile struct flash_comm_region_t flash __attribute__((section("shared"))) = {
    .command = {0},
    .response = {0},
    .command_sync = SYNC_UNINIT,
    .response_sync = SYNC_UNINIT
};
static volatile void* flash_data = ((void*)(FLASH_DATA_ADDR));

static int BoardInit(void);

int main(void)
{
    BoardInit();

    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    //tell the debugger stub is ready
    flash.response.data_size = 0;
    flash.response.retval = FLASH_RESPONSE_INITIALIZED;
    flash.response_sync = SYNC_READY;

    for(;;){
        while(flash.response_sync != SYNC_WAIT)
            ;
        while(flash.command_sync != SYNC_READY)
            ;

        switch(flash.command.type){
        case FD_OPEN:
            flash.response.retval = sl_FsOpen(
                    ((struct command_file_open_args_t*)flash_data)->pFileName,
                    ((struct command_file_open_args_t*)flash_data)->AccessModeAndMaxSize,
                    NULL,
                    ((struct command_file_open_args_t*)flash_data)->pFileHandle);
            break;
        case FD_CLOSE:
            flash.response.retval = sl_FsClose(
                    ((struct command_file_close_args_t*)flash_data)->FileHdl,
                    NULL, NULL, 0);
            break;
        case FD_READ:
            flash.response.retval = sl_FsRead(
                    ((struct command_file_read_args_t*)flash_data)->FileHdl,
                    ((struct command_file_read_args_t*)flash_data)->Offset,
                    ((struct command_file_read_args_t*)flash_data)->pData,
                    ((struct command_file_read_args_t*)flash_data)->Len);
            break;
        case FD_WRITE:
            flash.response.retval = sl_FsWrite(
                    ((struct command_file_write_args_t*)flash_data)->FileHdl,
                    ((struct command_file_write_args_t*)flash_data)->Offset,
                    ((struct command_file_write_args_t*)flash_data)->pData,
                    ((struct command_file_write_args_t*)flash_data)->Len);
            break;
        case FD_DELETE:
            flash.response.retval = sl_FsDel(
                    ((struct command_file_delete_args_t*)flash_data)->pFileName,
                    0);
            break;
        default:
            break;
        }

        flash.response_sync = SYNC_READY;
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
