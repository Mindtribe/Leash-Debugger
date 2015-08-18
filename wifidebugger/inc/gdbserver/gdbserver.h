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

#define MAX_GDB_PACKET_LEN 100

int gdbserver_init(void);

int gdbserver_processChar(void);

int gdbserver_processPacket(void);

void gdbserver_reset_error(int line, int error_code);

int gdbserver_loop(void);

#endif
