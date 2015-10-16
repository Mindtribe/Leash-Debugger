/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "freertos_hooks.h"
#include "error.h"
#include "led.h"
#include "ui.h"

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    (void)xTask; //prevent unused warning
    (void)pcTaskName; //prevent unused warning

    SetLEDBlink(LED_RED, LED_BLINK_PATTERN_ERROR);
    while(1);
}

void vApplicationTickHook( void )
{

}

void vApplicationIdleHook( void )
{

}

void vApplicationMallocFailedHook()
{
    SetLEDBlink(LED_RED, LED_BLINK_PATTERN_ERROR);
    while(1);
}

