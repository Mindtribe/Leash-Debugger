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

int gdb_helpers_init(void (*pPutChar)(char), void (*pGetChar)(char*), int (*pGetCharsAvail)(void));

void gdb_helpers_PutChar(char c);

void gdb_helpers_GetChar(char* c);

void gdb_helpers_Ack(void);

void gdb_helpers_Nack(void);

void gdb_helpers_TransmitPacket(char* packet_data);

int gdb_helpers_isInitialized(void);

int gdb_helpers_CharsAvaliable(void);

void gdb_helpers_hexStrToStr(char* hexStr, char* str);

#endif
