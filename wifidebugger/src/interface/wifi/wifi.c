/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

//FreeRTOS includes
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

//SimpleLink includes
#include "simplelink.h"

//project includes
#include "wifi.h"
#include "common.h"
#include "error.h"

#define SPAWNQUEUE_SIZE (3)

#define SPAWNTASK_STACK_SIZE (2048)
#define SPAWNTASK_PRIORITY (9)

//SimpleLink spawn message type: specifies
//a callback and parameter pointer.
typedef struct
{
    void (*pEntry)(void*);
    void* pValue;
}tSLSpawnMessage;

//SimpleLink spawn queue, for receiving
//spawn messages
QueueHandle_t SLSpawnQueue = NULL;

//Handle to the spawn task
TaskHandle_t SLSpawnTask = NULL;

int WifiInit(void)
{
    return RET_SUCCESS;
}

int WifiDeleteSpawnTask(void)
{
    if(SLSpawnTask != NULL){
        vTaskDelete(SLSpawnTask);
        SLSpawnTask = NULL;
    }
    if(SLSpawnQueue != NULL){
        vQueueDelete(SLSpawnQueue);
        SLSpawnQueue = NULL;
    }
    return RET_SUCCESS;
}

int WifiStartSpawnTask(void)
{
    if(SLSpawnTask != NULL || SLSpawnQueue != NULL){
        RETURN_ERROR(ERROR_UNKNOWN);
    }

    SLSpawnQueue = xQueueCreate(SPAWNQUEUE_SIZE, sizeof(tSLSpawnMessage));
    if(SLSpawnQueue == NULL){
        RETURN_ERROR(ERROR_UNKNOWN);
    }

    if(xTaskCreate(Task_SLSpawn,
            (portCHAR*) "SL_Spawn",
            (SPAWNTASK_STACK_SIZE/sizeof(portSTACK_TYPE)),
            NULL,
            SPAWNTASK_PRIORITY,
            &SLSpawnTask)
            != pdTRUE){
        RETURN_ERROR(ERROR_UNKNOWN);
    }

    return RET_SUCCESS;
}

void Task_WifiScan(void* params)
{
    (void)params; //avoid unused error
    return;
}


void Task_SLSpawn(void *params)
{
    (void)params; //avoid unused warning

    tSLSpawnMessage Msg;
    portBASE_TYPE ret=pdFAIL;

    //infinitely wait for spawn messages from the queue,
    //and spawn the functions specified inside.
    for(;;)
    {
        ret = xQueueReceive( SLSpawnQueue, &Msg, portMAX_DELAY );
        if(ret == pdPASS)
        {
                Msg.pEntry(Msg.pValue);
        }
    }
}
