/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef WIFI_H_
#define WIFI_H_

#include "simplelink.h"

#define WIFI_TASK_STACK_SIZE 2048
#define WIFI_TASK_PRIORITY 1
#define WIFI_NUM_NETWORKS 20

typedef struct wifi_state_t {
    SlVersionFull version;
    unsigned char status;
    unsigned long station_IP;
    Sl_WlanNetworkEntry_t networks[WIFI_NUM_NETWORKS];
}wifi_state_t;
extern struct wifi_state_t wifi_state;

int WifiInit(void); //initialize and start spawn task
int WifiDeleteSpawnTask(void); //delete the spawn task and queue
int WifiStartSpawnTask(void); //create the spawn task and queue
int WifiStartDefaultSettings(void); //go to default state and given mode

//FreeRTOS tasks
void Task_WifiScan(void* params); //scan for WiFi networks
void Task_SLSpawn(void* params); //message queue spawn task

#endif
