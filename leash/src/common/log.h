/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

/*
Logging functionality
 */

#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include "FreeRTOS.h"
#include "semphr.h"

#define LOG_LEVEL LOG_VERBOSE

//declaring FreeRTOS dynamic memory management
void* pvPortMalloc(size_t);
void vPortFree(void*);

enum log_level{
    LOG_VERBOSE = 0,
    LOG_IMPORTANT,
    LOG_ERROR,
    LOG_HIGHEST,
};

#define MAX_MEM_LOG_LEN (64)
#define MAX_MEM_LOG_ENTRIES (30)

#define LOG(X, ...) { \
    if((X) >= LOG_LEVEL){ \
        char pTempLogMsg[MAX_MEM_LOG_LEN]; \
        snprintf(pTempLogMsg, MAX_MEM_LOG_LEN, __VA_ARGS__); \
        log_put(pTempLogMsg); \
    } \
}

void log_put(char* msg);
void mem_log_clear(void);

//for outputting log messages over an arbitraty putchar interface.
void mem_log_start_putchar(void (*putchar)(char));
void mem_log_stop_putchar(void);

#endif
