/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "error.h"

#include "gpio_if.h"
#include "uart_if.h"

#include "gdb_helpers.h"

#include "wfd_conversions.h"
#include "wfd_string.h"

#define ERROR_CODECHAR_MAX 20
#define ERROR_LINECHAR_MAX 10
#define FILE_MAX 100
#define MAX_ERROR_LOGS 20

struct error_log{
    int line;
    uint32_t code;
    char file[FILE_MAX];
    char codechar[ERROR_CODECHAR_MAX];
    char linechar[ERROR_LINECHAR_MAX];
};

struct error_state_t{
    int cur_error;
    int overflow;
    struct error_log errors[MAX_ERROR_LOGS];
};

static struct error_state_t error_state = {
    .cur_error = 0,
    .overflow = 0
};

void error_wait(char* file, int line, uint32_t error_code)
{
    error_add(file, line, error_code);

    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    while(1){};
    return;
}

void error_add(char* file, int line, uint32_t error_code)
{
    if(error_state.cur_error<MAX_ERROR_LOGS){
        wfd_strncpy(error_state.errors[error_state.cur_error].file, file, FILE_MAX);
        wfd_itoa(error_code, error_state.errors[error_state.cur_error].codechar);
        wfd_itoa(line, error_state.errors[error_state.cur_error].linechar);
        error_state.errors[error_state.cur_error].line = line;
        error_state.errors[error_state.cur_error].code = error_code;

        error_state.cur_error++;
    }
    else error_state.overflow = 1;

    char msg[100];
    int msgi = 0;
    msgi += wfd_strncpy(&(msg[msgi]), "O Error ", 100);
    msgi += wfd_strncpy(&(msg[msgi]), error_state.errors[error_state.cur_error - 1].codechar, 100);
    msgi += wfd_strncpy(&(msg[msgi]), " @ ", 100);
    msgi += wfd_strncpy(&(msg[msgi]), error_state.errors[error_state.cur_error - 1].file, 100);
    msgi += wfd_strncpy(&(msg[msgi]), ":", 100);
    msgi += wfd_strncpy(&(msg[msgi]), error_state.errors[error_state.cur_error - 1].linechar, 100);

    return;
}

void clear_errors(void)
{
    for(int i=0; i<MAX_ERROR_LOGS; i++){
        error_state.errors[i].file[0] = 0;
        error_state.errors[i].line = 0;
        error_state.errors[i].code = 0;
    }
    error_state.cur_error = 0;
    error_state.overflow = 0;

    return;
}
