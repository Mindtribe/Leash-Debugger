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

#include "common.h"

#define ERROR_UNKNOWN 1

#define MAX_ERROR_LOGS 20

#define RETURN_ERROR(X) {error_add(__FILE__,__LINE__,X); return RET_FAILURE;}
#define WAIT_ERROR(X) {error_wait(__FILE__,__LINE__,X);}

void error_wait(char* file, int line, uint32_t error_code);
void error_add(char* file, int line, uint32_t error_code);
void clear_errors(void);

#endif
