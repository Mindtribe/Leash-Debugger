/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "freertos_hooks.h"
#include "error.h"

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    (void)xTask; //prevent unused warning
    (void)pcTaskName; //prevent unused warning
    error_wait(__FILE__, __LINE__, ERROR_UNKNOWN);
}

void vApplicationTickHook( void )
{

}

void vApplicationIdleHook( void )
{

}

void vApplicationMallocFailedHook()
{
    error_wait(__FILE__, __LINE__, ERROR_UNKNOWN);
}

