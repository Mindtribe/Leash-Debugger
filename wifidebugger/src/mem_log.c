/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "mem_log.h"
#include "uart_if.h"

#include "common.h"
#include "gdb_helpers.h"

struct mem_log_entry{
    char msg[MSG_MAX];
    char codechar[CODECHAR_MAX];
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
        mem_log_state.log[i].msg[0] = 0;
        mem_log_state.log[i].code = 0;
    }

    mem_log_state.cur_entry = 0;
    return;
}

void mem_log_add(char* msg, int code){
    if(mem_log_state.cur_entry < MEM_LOG_ENTRIES){
        wfd_strncpy(mem_log_state.log[mem_log_state.cur_entry].msg, msg, MSG_MAX);
        wfd_itoa(code, mem_log_state.log[mem_log_state.cur_entry].codechar);
        mem_log_state.log[mem_log_state.cur_entry].code = code;
    }
    else mem_log_state.overflow = 1;
    mem_log_state.cur_entry++;

    char msgs[100];
    int msgi = 0;
    msgi += wfd_strncpy(&(msgs[msgi]), "O Log:  ", 100);
    msgi += wfd_strncpy(&(msgs[msgi]), mem_log_state.log[mem_log_state.cur_entry-1].msg, 100);
    msgi += wfd_strncpy(&(msgs[msgi]), " (code ", 100);
    msgi += wfd_strncpy(&(msgs[msgi]), mem_log_state.log[mem_log_state.cur_entry-1].codechar, 100);
    msgi += wfd_strncpy(&(msgs[msgi]), ")\n\r", 100);

    if(!gdb_helpers_isInitialized()){
        Message(&(msgs[2]));
    }
    else{
        //gdb_helpers_TransmitPacket(msgs); //enable to send logs to GDB terminal.
    }

    return;
}
