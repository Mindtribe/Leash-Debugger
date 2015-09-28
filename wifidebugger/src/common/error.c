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

#include <stddef.h>
#include <stdlib.h>

void error_wait(char* file, int line, uint32_t error_code)
{
    error_add(file, line, error_code);

    while(1){};
    return;
}

void error_add(char* file, int line, uint32_t error_code)
{
    LOG(LOG_ERROR, "[ERROR %d] @ %s:%d", (unsigned int) error_code, file, (unsigned int) line);

    SetLEDBlink(LED_RED, LED_BLINK_PATTERN_ERROR);

    return;
}
