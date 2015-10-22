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
#include "semphr.h"
#include "task.h"

#include "gdb_helpers.h"
#include "mem.h"

#define USE_MEM_LOG
#define USE_PUTCHAR_LOG

#define MEM_LOG_FLAG_VALID 1
#define MEM_LOG_FLAG_FLUSHED 2

#define MEMLOG_TASK_STACK_SIZE (2048)
#define MEMLOG_TASK_PRIORITY (2)

#ifndef NULL
#define NULL 0
#endif

static void Task_PutLogMessages(void* params);

static void default_putchar(char c)
{
    (void)c;
    return;
}

struct mem_log_entry{
    char* msg;
    unsigned int flags;
};
struct mem_log_state_t{
    int cur_entry;
    int overflow;
    struct mem_log_entry memlog[MAX_MEM_LOG_ENTRIES];
    char mem_log_msg[MAX_MEM_LOG_ENTRIES][MAX_MEM_LOG_LEN];
    SemaphoreHandle_t mutex;
    void (*msg_putchar)(char);
    unsigned int initialized;
    QueueHandle_t log_msg_put_queue;
    unsigned int stack_watermark;
};

static struct mem_log_state_t mem_log_state = {
    .cur_entry = 0,
    .overflow = 0,
    .memlog = {{0}},
    .mutex = 0,
    .msg_putchar = &default_putchar,
    .initialized = 0,
    .log_msg_put_queue = 0,
    .stack_watermark = 0xFFFFFFFF,
    .mem_log_msg = {{0}}
};

struct mem_log_entry* mem_log_add(char* msg);

static struct mem_log_entry overflow_entry = {
    .msg = "Note: Mem log overflow. Message(s) dropped.",
    .flags = 0
};

static void flush_log_messages(void)
{
    //flush all past messages onto the log channel.
    for(int i=0; i<MAX_MEM_LOG_ENTRIES; i++){
        if((mem_log_state.memlog[i].flags & MEM_LOG_FLAG_VALID)
                && !(mem_log_state.memlog[i].flags & MEM_LOG_FLAG_FLUSHED)){
            void* pEntry = (void*) &(mem_log_state.memlog[i]);
            xQueueSend(mem_log_state.log_msg_put_queue,
                    (void*) &(pEntry),
                    portMAX_DELAY);
            mem_log_state.memlog[i].flags |= MEM_LOG_FLAG_FLUSHED;
        }
    }
}

static void Task_PutLogMessages(void* params)
{
    (void)params;
    struct mem_log_entry *entry;
    for(;;){
        xQueueReceive(
                mem_log_state.log_msg_put_queue,
                (void*) (&entry),
                portMAX_DELAY);
        for(char*c = entry->msg; *c!=0; c++){
            (*mem_log_state.msg_putchar)(*c);
        }
        mem_log_state.msg_putchar('\n');
        entry->flags |= MEM_LOG_FLAG_FLUSHED;
        mem_log_state.stack_watermark = uxTaskGetStackHighWaterMark(NULL);
    }
    vTaskDelete(NULL);
}

void mem_log_init(){
    mem_log_state.mutex = xSemaphoreCreateMutex();
    vQueueAddToRegistry(mem_log_state.mutex ,"Memlog mutex");
    mem_log_state.initialized = 1;

    mem_log_state.log_msg_put_queue = xQueueCreate(MAX_MEM_LOG_ENTRIES, sizeof(struct mem_log_entry*));
    vQueueAddToRegistry(mem_log_state.log_msg_put_queue ,"LogMsgPut");

    for(int i=0; i<MAX_MEM_LOG_ENTRIES; i++){
        mem_log_state.memlog[i].msg = (char*)&(mem_log_state.mem_log_msg[i]);
    }

    xTaskCreate(Task_PutLogMessages,
            "Mem Log Put",
            MEMLOG_TASK_STACK_SIZE/sizeof(portSTACK_TYPE),
            0,
            MEMLOG_TASK_PRIORITY,
            0);
}

void log_put(char* msg)
{
    if(!mem_log_state.initialized) mem_log_init();

#ifdef USE_MEM_LOG
    //add it to the memory log
    struct mem_log_entry* entry = mem_log_add(msg);
#ifdef USE_PUTCHAR_LOG
    if(mem_log_state.msg_putchar != default_putchar){
        void* pEntry = (void*)entry;
        xQueueSend(
                mem_log_state.log_msg_put_queue,
                (void*)&pEntry,
                portMAX_DELAY);
    }
#endif //USE_PUTCHAR_LOG
#endif //USE_MEM_LOG
}

void mem_log_start_putchar(void (*putchar)(char))
{
    if(!mem_log_state.initialized) mem_log_init();
    xSemaphoreTake(mem_log_state.mutex, portMAX_DELAY);

    mem_log_state.msg_putchar = putchar;

#ifdef USE_PUTCHAR_LOG
    flush_log_messages();
#endif

    xSemaphoreGive(mem_log_state.mutex);
    return;
}

void mem_log_stop_putchar(void)
{
    if(!mem_log_state.initialized) mem_log_init();
    xSemaphoreTake(mem_log_state.mutex, portMAX_DELAY);

    mem_log_state.msg_putchar = &default_putchar;

    xSemaphoreGive(mem_log_state.mutex);
    return;
}

void mem_log_clear(void)
{
    if(!mem_log_state.initialized) mem_log_init();
    xSemaphoreTake(mem_log_state.mutex, portMAX_DELAY);

    for(int i=0; i<MAX_MEM_LOG_ENTRIES; i++){
        mem_log_state.memlog[i].flags = 0;
        mem_log_state.memlog[i].msg[0] = 0;
    }

    mem_log_state.cur_entry = 0;
    mem_log_state.overflow = 0;

    xSemaphoreGive(mem_log_state.mutex);
    return;
}

struct mem_log_entry* mem_log_add(char* msg)
{
    if(!mem_log_state.initialized) mem_log_init();
    xSemaphoreTake(mem_log_state.mutex, portMAX_DELAY);

    if(mem_log_state.cur_entry >= MAX_MEM_LOG_ENTRIES){
        mem_log_state.cur_entry = 0;
    }

    if(mem_log_state.memlog[mem_log_state.cur_entry].flags & MEM_LOG_FLAG_VALID){
        if(!(mem_log_state.memlog[mem_log_state.cur_entry].flags & MEM_LOG_FLAG_FLUSHED)){
            mem_log_state.overflow = 1;
        }
    }

    if(mem_log_state.overflow == 1){
        if(mem_log_state.msg_putchar != &default_putchar){
            void* pEntry = (void*)&overflow_entry;
            xQueueSend(
                    mem_log_state.log_msg_put_queue,
                    (void*)&pEntry,
                    portMAX_DELAY);
            mem_log_state.overflow = 0;
        }
    }

    int len = strlen(msg)+1;
    if(len>MAX_MEM_LOG_LEN){
        //truncate string with "(...)"
        msg[MAX_MEM_LOG_LEN-1] = 0;
        msg[MAX_MEM_LOG_LEN-2] = ')';
        msg[MAX_MEM_LOG_LEN-3] = '.';
        msg[MAX_MEM_LOG_LEN-4] = '.';
        msg[MAX_MEM_LOG_LEN-5] = '.';
        msg[MAX_MEM_LOG_LEN-6] = '(';
        len = strlen(msg)+1;
    }

    mem_log_state.memlog[mem_log_state.cur_entry].msg[0] = 0;
    strncpy(mem_log_state.memlog[mem_log_state.cur_entry].msg, msg, len);
    mem_log_state.memlog[mem_log_state.cur_entry].flags = MEM_LOG_FLAG_VALID;

    mem_log_state.cur_entry++;

    xSemaphoreGive(mem_log_state.mutex);

    return &(mem_log_state.memlog[mem_log_state.cur_entry-1]);
}
