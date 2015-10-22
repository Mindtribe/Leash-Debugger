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

#include <stdint.h>

#define GDBSERVER_TASK_STACK_SIZE (4096)
#define GDBSERVER_TASK_PRIORITY (5)

struct target_al_interface;

//Initialize GDBServer. This needs to be called only once on startup.
//pPutChar, pGetChar and pGetCharsAvail are function callbacks used by GDB to communicate.
//target is an abstract target interface structure, to be filled in for a specific type of target.
int gdbserver_init(void (*pPutChar)(char), void (*pGetChar)(char*), int (*pGetCharsAvail)(void), struct target_al_interface *target);

//After calling gdbserver_init, start this function as a FreeRTOS task for
//handling all GDBServer-related matters.
void Task_gdbserver(void* params);

#endif
