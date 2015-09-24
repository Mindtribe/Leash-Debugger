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
#define WIFI_TASK_PRIORITY 4
#define WIFI_NUM_NETWORKS 20

//struct that holds parameters of an AP to connect to
//in station mode.
struct tApParams{
    char* ssid;
    SlSecParams_t secparams;
};

struct wifi_state_t {
    SlVersionFull version;
    unsigned char status;
    unsigned long client_IP;
    unsigned long self_IP;
    Sl_WlanNetworkEntry_t networks[WIFI_NUM_NETWORKS];
    struct tApParams ap;
    unsigned int startAP;
};
extern struct wifi_state_t wifi_state;

int WifiInit(unsigned int startAP); //initialize and start spawn task (if startAP is nonzero, start in AP mode)
int WifiDeleteSpawnTask(void); //delete the spawn task and queue
int WifiStartSpawnTask(void); //create the spawn task and queue
int WifiStartDefaultSettings(void); //go to default state and given mode

//FreeRTOS tasks
void Task_WifiScan(void* params); //scan for WiFi networks
void Task_SLSpawn(void* params); //message queue spawn task
void Task_Wifi(void* params); //Encompassing task for all WiFi operations.
void Task_WifiAP(void* params); //Task for having WiFi in AP mode.
void Task_WifiSTA(void* params); //Task for having WiFi in Station mode.

#endif
