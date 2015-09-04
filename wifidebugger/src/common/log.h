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

#define LOG(X, ...) { \
    if((X) >= LOG_LEVEL){ \
        char* pLogPtr = (char*) pvPortMalloc( (size_t) (snprintf(NULL,0,__VA_ARGS__)+1) ); \
        if(pLogPtr != NULL){ \
            sprintf(pLogPtr, __VA_ARGS__); \
            log_put(pLogPtr); \
            vPortFree(pLogPtr); \
        } \
    } \
}

void log_put(char* msg);
void mem_log_add(char* msg);
void mem_log_clear(void);

#endif
