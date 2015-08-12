/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "mem_log.h"

struct mem_log_entry{
    char* msg;
    int code;
};

struct mem_log_state_t{
    int cur_entry;
    int overflow;
    struct mem_log_entry log[MEM_LOG_ENTRIES];
};

static struct mem_log_state_t mem_log_state = {
    .cur_entry = 0,
    .overflow = 0
};

void mem_log_clear(void){
    for(int i=0; i<MEM_LOG_ENTRIES; i++){
        mem_log_state.log[i].msg = 0;
        mem_log_state.log[i].code = 0;
    }

    mem_log_state.cur_entry = 0;
    return;
}

void mem_log_add(char* msg, int code){
    if(mem_log_state.cur_entry < MEM_LOG_ENTRIES){
        mem_log_state.log[mem_log_state.cur_entry].msg = msg;
        mem_log_state.log[mem_log_state.cur_entry].code = code;
    }
    else mem_log_state.overflow = 1;
    mem_log_state.cur_entry++;

    return;
}
