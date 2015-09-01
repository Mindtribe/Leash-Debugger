/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

#define RET_SUCCESS (0)
#define RET_FAILURE (-1)

#define USE_FREERTOS //required for TI's startup file

#define MIN(X, Y) ((X > Y) ? Y : X)

#define WFD_NAME_STRING ""\
    "\n"\
    "------------------------------------------\n"\
    "CC3200 Wi-Fi Debugger v0.1\n"\
    "Copyright 2015, MindTribe inc.\n"\
    "------------------------------------------\n"\
    "\n"\
    "Note: this is a work-in-progress.\n"\
    "Please see KNOWN_BUGS before using.\n"\
    "\n"

#ifndef NULL
#define NULL 0
#endif

#endif
