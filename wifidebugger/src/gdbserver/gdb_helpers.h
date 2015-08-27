/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

/*
 * Helper functions for the GDB server implementation.
 *
 */

#ifndef GDB_HELPERS_H_
#define GDB_HELPERS_H_

#include <stdint.h>

#define CRC32_POLY 0x04C11DB7

int gdb_helpers_init(void (*pPutChar)(char), void (*pGetChar)(char*), int (*pGetCharsAvail)(void));

void gdb_helpers_PutChar(char c);

void gdb_helpers_GetChar(char* c);

void gdb_helpers_Ack(void);

void gdb_helpers_Nack(void);

uint8_t gdb_helpers_getChecksum(char* data);

void gdb_helpers_TransmitPacket(char* packet_data);

int gdb_helpers_isInitialized(void);

int gdb_helpers_CharsAvaliable(void);

#endif