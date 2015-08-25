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

void default_PutChar(char c);
void default_GetChar(char* c);
int default_GetCharsAvailable(void);

struct gdb_helpers_state_t{
    int initialized;
    void (*pPutChar)(char);
    void (*pGetChar)(char*);
    int (*pGetCharsAvail)(void);
};
struct gdb_helpers_state_t gdb_helpers_state = {
    .initialized = 0,
    .pPutChar = &default_PutChar,
    .pGetChar = &default_GetChar,
    .pGetCharsAvail = &default_GetCharsAvailable
};

int default_GetCharsAvailable(void)
{
    //if we reach here, someone tried to do stuff
    //before proper pointer initialization.
    //throw an error!
    WAIT_ERROR(ERROR_UNKNOWN);

    return RET_FAILURE;
}

void default_PutChar(char c)
{
    //if we reach here, someone tried to send messages
    //before proper pointer initialization.
    //throw an error!
    WAIT_ERROR(ERROR_UNKNOWN);

    return;
}

void default_GetChar(char* c)
{
    //if we reach here, someone tried to receive messages
    //before proper pointer initialization.
    //throw an error!
    WAIT_ERROR(ERROR_UNKNOWN);

    return;
}

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
    if(!gdb_helpers_state.initialized) WAIT_ERROR(ERROR_UNKNOWN);

    gdb_helpers_state.pPutChar(c);

    return;
}

void gdb_helpers_GetChar(char* c)
{
    if(!gdb_helpers_state.initialized) WAIT_ERROR(ERROR_UNKNOWN);

    gdb_helpers_state.pGetChar(c);

    return;
}

int gdb_helpers_CharsAvaliable(void)
{
    if(!gdb_helpers_state.initialized) WAIT_ERROR(ERROR_UNKNOWN);

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

void gdb_helpers_byteToHex(uint8_t byte, char* dst)
{
    wfd_byteToHex(byte, dst);
}

uint8_t gdb_helpers_hexToByte(char* src){

    return wfd_hexToByte(src);
}

void gdb_helpers_toHex(char* src, char* dst)
{
    int i;
    for(i=0; src[i] != 0; i++){
        gdb_helpers_byteToHex((uint8_t)src[i], &(dst[2*i]));
    }
    dst[2*i] = 0;

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
    char chksm_nibbles[2];
    gdb_helpers_byteToHex(checksum, chksm_nibbles);
    gdb_helpers_state.pPutChar(chksm_nibbles[0]);
    gdb_helpers_state.pPutChar(chksm_nibbles[1]);

    return;
}

int gdb_helpers_isInitialized(void){
    return gdb_helpers_state.initialized;
}

int gdb_helpers_isHex(char c)
{
    if((c>='0') && (c<='9')) return 1;
    if((c>='A') && (c<='F')) return 1;
    if((c>='a') && (c<='f')) return 1;
    return -1;
}

char gdb_helpers_toUpperCaseHex(char c)
{
    return wfd_toUpperCaseHex(c);
}

uint32_t gdb_helpers_hexToInt(char* src)
{
    return wfd_hexToInt(src);
}

uint32_t gdb_helpers_hexToInt_LE(char* src)
{
    uint32_t data = wfd_hexToInt(src);
    return flip_endian(data);
}
