/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "gdb_helpers.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "error.h"

struct gdb_helpers_state_t{
    int initialized;
    void (*pPutChar)(char);
    void (*pGetChar)(char*);
    int (*pGetCharsAvail)(void);
};
struct gdb_helpers_state_t gdb_helpers_state = {
    .initialized = 0
};

int gdb_helpers_init(void (*pPutChar)(char), void (*pGetChar)(char*), int (*pGetCharsAvail)(void))
{
    gdb_helpers_state.pPutChar = pPutChar;
    gdb_helpers_state.pGetChar = pGetChar;
    gdb_helpers_state.pGetCharsAvail = pGetCharsAvail;
    gdb_helpers_state.initialized = 1;
    return RET_SUCCESS;
}

void gdb_helpers_PutChar(char c)
{
    if(!gdb_helpers_state.initialized) {WAIT_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    gdb_helpers_state.pPutChar(c);

    return;
}

void gdb_helpers_GetChar(char* c)
{
    if(!gdb_helpers_state.initialized) {WAIT_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    gdb_helpers_state.pGetChar(c);

    return;
}

int gdb_helpers_CharsAvaliable(void)
{
    if(!gdb_helpers_state.initialized) {WAIT_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    return gdb_helpers_state.pGetCharsAvail();
}

void gdb_helpers_Ack(void)
{
    gdb_helpers_state.pPutChar('+');
    return;
}

void gdb_helpers_Nack(void)
{
    gdb_helpers_state.pPutChar('-');
    return;
}

void gdb_helpers_TransmitPacket(char* packet_data)
{
    uint8_t checksum = 0;
    gdb_helpers_state.pPutChar('$');
    for(int i=0; packet_data[i] != 0; i++){
        gdb_helpers_PutChar(packet_data[i]);
        checksum += packet_data[i];
    }
    gdb_helpers_state.pPutChar('#');
    //put the checksum
    char chksm_nibbles[3];
    sprintf(chksm_nibbles, "%02X", checksum);
    gdb_helpers_state.pPutChar(chksm_nibbles[0]);
    gdb_helpers_state.pPutChar(chksm_nibbles[1]);

    return;
}

int gdb_helpers_isInitialized(void){
    return gdb_helpers_state.initialized;
}

void gdb_helpers_hexStrToStr(char* hexStr, char* str)
{
    unsigned int i;
    for(i=0; (i*2)<strlen(hexStr); i++){
        unsigned int val;
        sscanf(&(hexStr[i*2]), "%2X", &val);
        str[i] = (char)val;
    }
    str[i] = 0;
    return;
}

