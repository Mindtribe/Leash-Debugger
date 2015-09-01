/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */


#include <stdio.h>
#include <stdint.h>

#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"

#include "gpio_if.h"
#include "pin_mux_config.h"

struct semihostArg{
    uint32_t word1;
    uint32_t word2;
    uint32_t word3;
    uint32_t word4;
};

static int BoardInit(void);
void semihost_printstr(char* msg);
int semihost_getinput(char* buf, uint32_t len);
void wfd_wordToHex(uint32_t word, char* dst);
void wfd_byteToHex(uint8_t byte, char* dst);

#define SEMIHOST_BUF_SIZE 256

int main(void)
{
    BoardInit();

    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);

    semihost_printstr("\n\nTestApp: I am alive! :)\nNow, let me echo you.\n>");

    char buffer[SEMIHOST_BUF_SIZE];
    int result;

    for(;;){
        GPIO_IF_LedToggle(MCU_RED_LED_GPIO);

        result = semihost_getinput(buffer, SEMIHOST_BUF_SIZE);
        char s_codereply[] = "Success ________:\n";
        char f_codereply[] = "Failed ________!\n>";

        if(result != -1){
            wfd_wordToHex((uint32_t) result, &(s_codereply[8]));
            semihost_printstr(s_codereply);
            semihost_printstr(buffer);
            semihost_printstr("\n>");
        }
        else{
            wfd_wordToHex((uint32_t) result, &(f_codereply[7]));
            semihost_printstr(f_codereply);
        }
    }

    return 1;
}

static int BoardInit(void)
{
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);
    PRCMCC3200MCUInit();

    PinMuxConfig();
    GPIO_IF_LedConfigure(LED1|LED2|LED3);
    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    return 1;
}

void semihost_printstr(char* msg)
{
    //set the arguments
    asm volatile("mov R0,#0x04");
    asm volatile("mov R1,%[message]" : : [message]"r" (msg));

    //break and let debugger do its magic
    asm volatile("BKPT 0xAB");

    //TODO: if wanting to error-check, read R0 here

    return;
}

int semihost_getinput(char* buf, uint32_t len)
{
    struct semihostArg arg;
    arg.word1 = 0;
    arg.word2 = (uint32_t)buf;
    arg.word3 = len;

    for(uint32_t i=0; i<len; i++) {buf[i]=0;}

    int response = 0;

    //set the arguments
    asm volatile("mov R0, #0x06");
    asm volatile("mov R1, %[arguments]" : : [arguments]"r" (&arg));

    //break and let the debugger do its magic
    asm volatile("BKPT 0xAB");

    //read the response
    asm volatile("mov %[result], R0" : [result]"=r" (response));

    return response;
}

void wfd_wordToHex(uint32_t word, char* dst)
{
    wfd_byteToHex((uint8_t)(word>>24), &(dst[0]));
    wfd_byteToHex((uint8_t)(word>>16), &(dst[2]));
    wfd_byteToHex((uint8_t)(word>>8), &(dst[4]));
    wfd_byteToHex((uint8_t)(word>>0), &(dst[6]));
}

void wfd_byteToHex(uint8_t byte, char* dst)
{
    char* hNibble = dst;
    char* lNibble = &(dst[1]);
    uint8_t i_lNibble, i_hNibble;
    i_lNibble = byte%16;
    i_hNibble = byte/16;

    if(i_lNibble < 10) *lNibble = '0' + (char)i_lNibble;
    else *lNibble = 'A' + ((char)i_lNibble-10);
    if(i_hNibble < 10) *hNibble = '0' + (char)i_hNibble;
    else *hNibble = 'A' + ((char)i_hNibble-10);
    return;
}
