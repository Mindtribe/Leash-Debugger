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

#include <stdint.h>

#ifndef ERROR_H_
#define ERROR_H_

enum error_type{
    ERROR_UNKNOWN = 0,
    ERROR_OVERFLOW
};

#define RET_SUCCESS (0)
#define RET_FAILURE (-1)

#define RETURN_ERROR(NUM, DESC) {error_add(NUM, DESC); return RET_FAILURE;}
#define TASK_RETURN_ERROR(NUM, DESC) {error_add(NUM, DESC); vTaskDelete(NULL); return; }
#define ADD_ERROR(NUM, DESC) {error_add(NUM, DESC);}
#define WAIT_ERROR(NUM, DESC) {error_wait(NUM, DESC);}

void error_wait(uint32_t error_code, const char* description);
void error_add(uint32_t error_code, const char* description);

#endif
