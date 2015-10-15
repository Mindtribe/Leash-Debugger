/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#ifndef USER_H_
#define USER_H_

#ifdef SIMPLELINK_FREERTOS_MULTITHREAD_FULL
#include "user_freertos_multithread_full.h"
#endif

#ifdef SIMPLELINK_NONOS_SINGLETHREAD_TINY
#include "user_nonos_singlethread_tiny.h"
#endif

#endif
