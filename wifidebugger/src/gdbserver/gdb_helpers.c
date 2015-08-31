/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include <stdint.h>

#include "error.h"
#include "gdb_helpers.h"
#include "wfd_conversions.h"

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
    if(!gdb_helpers_state.initialized) {WAIT_ERROR(ERROR_UNKNOWN);}

    gdb_helpers_state.pPutChar(c);

    return;
}

void gdb_helpers_GetChar(char* c)
{
    if(!gdb_helpers_state.initialized) {WAIT_ERROR(ERROR_UNKNOWN);}

    gdb_helpers_state.pGetChar(c);

    return;
}

int gdb_helpers_CharsAvaliable(void)
{
    if(!gdb_helpers_state.initialized) {WAIT_ERROR(ERROR_UNKNOWN);}

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

uint8_t gdb_helpers_getChecksum(char* data)
{
    uint8_t checksum = 0;
    for(int i=0; data[i] != 0; i++){
        checksum += (uint8_t)(data[i]);
    }

    return checksum;
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
    char chksm_nibbles[2];
    wfd_byteToHex(checksum, chksm_nibbles);
    gdb_helpers_state.pPutChar(chksm_nibbles[0]);
    gdb_helpers_state.pPutChar(chksm_nibbles[1]);

    return;
}

int gdb_helpers_isInitialized(void){
    return gdb_helpers_state.initialized;
}

