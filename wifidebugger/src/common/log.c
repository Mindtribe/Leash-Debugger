/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "log.h"

#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "FreeRTOS.h"

#include "gdb_helpers.h"
#include "mem.h"

#define MAX_MEM_LOG_LEN (256)
#define MAX_MEM_LOG_ENTRIES (((HEAP_SIZE)/4)/(MAX_MEM_LOG_LEN))

#define USE_MEM_LOG

char* pLogPtr;

struct mem_log_entry{
    char* msg;
    int valid;
};
struct mem_log_state_t{
    int cur_entry;
    int overflow;
    struct mem_log_entry memlog[MAX_MEM_LOG_ENTRIES];
};

static struct mem_log_state_t mem_log_state = {
    .cur_entry = 0,
    .overflow = 0,
    .memlog = {{0}}
};

void log_put(char* msg)
{
    //add it to the memory log
#ifdef USE_MEM_LOG
    mem_log_add(msg);
#endif
}

void mem_log_clear(void){
    for(int i=0; i<MAX_MEM_LOG_ENTRIES; i++){
        if(mem_log_state.memlog[i].valid){
            mem_log_state.memlog[i].valid = 0;
            vPortFree(mem_log_state.memlog[i].msg);
            mem_log_state.memlog[i].msg = NULL;
        }
    }

    mem_log_state.cur_entry = 0;
    mem_log_state.overflow = 0;
    return;
}

void mem_log_add(char* msg){
    if(mem_log_state.cur_entry >= MAX_MEM_LOG_ENTRIES){
        mem_log_state.overflow = 1;
        mem_log_state.cur_entry = 0;
    }

    if(mem_log_state.memlog[mem_log_state.cur_entry].valid){
        vPortFree(mem_log_state.memlog[mem_log_state.cur_entry].msg);
    }

    int len = strlen(msg)+1;
    if(len>MAX_MEM_LOG_LEN){
        msg = "[LOG] (Omit log msg: too long)";
        len = strlen(msg)+1;
    }

    mem_log_state.memlog[mem_log_state.cur_entry].msg = (char*)pvPortMalloc(len);
    strncpy(mem_log_state.memlog[mem_log_state.cur_entry].msg, msg, len);
    mem_log_state.memlog[mem_log_state.cur_entry].valid = 1;

    mem_log_state.cur_entry++;

    return;
}
