/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

/*
Simple logging structure in memory for use with a debugger.
 */

#ifndef MEM_LOG_H_
#define MEM_LOG_H_

void mem_log_add(char* msg, int code);
void mem_log_clear(void);

#endif
