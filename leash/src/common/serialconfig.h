/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

//Interface for configuring the debug adapter using a terminal connection.

#ifndef SERIALCONFIG_H_
#define SERIALCONFIG_H_

void serialconfig_start(void (*get_char)(char*));

void serialconfig_stop(void);

#endif

