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

#define GDBSERVER_TASK_STACK_SIZE 2048
#define GDBSERVER_TASK_PRIORITY 5

struct target_al_interface;

int gdbserver_init(void (*pPutChar)(char), void (*pGetChar)(char*), int (*pGetCharsAvail)(void), struct target_al_interface *target);
void gdbserver_loop_task(void* params);

#endif
