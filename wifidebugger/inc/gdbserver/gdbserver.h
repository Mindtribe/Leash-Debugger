/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

/*
 * Implementation of a server for GDB remote debugging protocol.
 */

#ifndef GDBSERVER_H_
#define GDBSERVER_H_

#define GDBSERVER_KEEP_CHARS //for debugging: whether to keep track of chars received
#define GDBSERVER_KEEP_CHARS_NUM 100 //for debugging: number of chars to keep track of

#define GDBSERVER_LOG_PACKETS

#define MAX_GDB_PACKET_LEN 100

int gdbserver_init(void (*pPutChar)(char), void (*pGetChar)(char*));

int gdbserver_processChar(void);

int gdbserver_processPacket(void);

void gdbserver_reset_error(int line, int error_code);

int gdbserver_loop(void);

void gdbserver_TransmitPacket(char* packet_data);

#endif
