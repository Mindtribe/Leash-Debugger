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

//Initialize GDB helper functions.
//pGetCharsAvail, pPutChar and pGetChar are character putting and getting functions which will be used
//by GDBServer to communicate with GDB (for example UART, TCP...)
int gdb_helpers_init(void (*pPutChar)(char), void (*pGetChar)(char*), int (*pGetCharsAvail)(void));

//Can be used to send a character into the channel gdb helpers were initialized with.
void gdb_helpers_PutChar(char c);

//Can be used to get a character from the channel gdb helpers were initialized with.
void gdb_helpers_GetChar(char* c);

//Send 'ACK' to GDB.
void gdb_helpers_Ack(void);

//Send 'NACK' to GDB.
void gdb_helpers_Nack(void);

//Transmit a packet (handles creation of start, stop and checksum characters).
void gdb_helpers_TransmitPacket(char* packet_data);

//Check whether GDB helpers have been initialized.
int gdb_helpers_isInitialized(void);

//Can be used to get the number of characters waiting in the communication channel's
//receive buffer.
int gdb_helpers_CharsAvaliable(void);

//Convert a string, which contains a hexadecimally encoded ASCII string, back to
//raw ASCII format.
void gdb_helpers_hexStrToStr(char* hexStr, char* str);

#endif
