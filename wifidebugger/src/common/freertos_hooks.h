/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#ifndef FREERTOS_HOOKS_H_
#define FREERTOS_HOOKS_H_

#include "FreeRTOS.h"
#include "task.h"

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName );

void vApplicationTickHook( void );

void vApplicationIdleHook( void );

void vApplicationMallocFailedHook();

#endif
