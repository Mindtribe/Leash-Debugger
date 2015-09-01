/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef WIFI_H_
#define WIFI_H_

int WifiInit(void); //initialize and start spawn task
int WifiDeleteSpawnTask(void); //delete the spawn task and queue
int WifiStartSpawnTask(void); //create the spawn task and queue

//FreeRTOS tasks
void Task_WifiScan(void* params); //scan for WiFi networks
void Task_SLSpawn(void* params); //message queue spawn task

#endif
