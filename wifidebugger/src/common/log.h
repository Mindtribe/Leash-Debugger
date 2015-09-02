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

extern void* pvPortMalloc(size_t); //FreeRTOS malloc
extern void vPortFree(void*); //FreeRTOS free

enum log_level{
    LOG_VERBOSE = 0,
    LOG_IMPORTANT,
    LOG_ERROR,
    LOG_HIGHEST,
};

extern char* pLogPtr;
#define LOG(X, ...) { \
    if((X) >= LOG_LEVEL){ \
        pLogPtr = (char*) pvPortMalloc( (size_t) (snprintf(NULL,0,__VA_ARGS__)+1) ); \
        sprintf(pLogPtr, __VA_ARGS__); \
        log_put(pLogPtr); \
    } \
}

//vPortFree((void*) pLogPtr);

void log_put(char* msg);
void mem_log_add(char* msg);
void mem_log_clear(void);

#endif
