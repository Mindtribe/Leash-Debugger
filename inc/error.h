/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

/*
 * For error handling.
 *
 */

#ifndef ERROR_H_
#define ERROR_H_

#define ERROR_UNKNOWN 1

void error_wait(int error_code);

#endif
