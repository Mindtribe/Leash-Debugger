/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "error.h"

#include "log.h"
#include "led.h"
#include "gpio_if.h"
#include "ui.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdlib.h>

void error_wait(uint32_t error_code, const char* description)
{
    error_add(error_code, description);
    SetLEDBlink(LED_RED, LED_BLINK_PATTERN_ERROR);

    vTaskDelete(NULL);
    return;
}

void error_add(uint32_t error_code, const char* description)
{
    LOG(LOG_ERROR, "[ERROR %d] %s", (unsigned int) error_code, description);

    return;
}
