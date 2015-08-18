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

int gdb_helpers_init(void* pPutChar(char), void* pGetChar(char*));

void gdb_helpers_PutChar(char c);

void gdb_helpers_GetChar(char* c);

void gdb_helpers_Ack(void);

void gdb_helpers_Nack(void);

uint8_t gdb_helpers_getChecksum(char* data);

void gdb_helpers_byteToHex(uint8_t byte, char* dst);

uint8_t gdb_helpers_hexToByte(char* src);

void gdb_helpers_toHex(char* src, char* dst);

void gdb_helpers_TransmitPacket(char* packet_data);

#endif
