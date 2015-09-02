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
#include "simplelink_defs.h"

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

struct wifi_state_t wifi_state ={
    .version = {
        .NwpVersion = {0}
    },
    .status = 0,
    .station_IP = 0
};

int WifiInit(void)
{
    if(WifiStartSpawnTask() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}

int WifiDeinit(void)
{
    if(WifiDeleteSpawnTask() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
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

int WifiStartDefaultSettings(void)
{
    long retval = -1;
    unsigned char status = 0;
    unsigned char val = 1;

    unsigned char config, config_len;

    //filters
    _WlanRxFilterOperationCommandBuff_t filter_mask = {
        .Padding = {0}
    };

    //initialize the SimpleLink API
    retval = sl_Start(0,0,0);
    if(retval < 0){RETURN_ERROR(ERROR_UNKNOWN);}

    //set device in station mode
    if(retval != ROLE_STA){
        if(retval == ROLE_AP){ //we need to wait for an event before doing anything
            while(!IS_IP_ACQUIRED(status)){};
        }
        //change mode to Station
        retval = sl_WlanSetMode(ROLE_STA);
        if(retval < 0) {RETURN_ERROR(ERROR_UNKNOWN);}
        //restart
        retval = sl_Stop(0xFF);
        if(retval < 0) {RETURN_ERROR(ERROR_UNKNOWN);}
        retval = sl_Start(0,0,0);
        if(retval < 0) {RETURN_ERROR(ERROR_UNKNOWN);}

        if(retval != ROLE_STA) {RETURN_ERROR(ERROR_UNKNOWN);}
    }

    //get SimpleLink version
    config = SL_DEVICE_GENERAL_VERSION;
    config_len = sizeof(SlVersionFull);
    retval = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &config, &config_len, (unsigned char*)&wifi_state.version);
    if(retval<0) {RETURN_ERROR(ERROR_UNKNOWN);}

    //default connection policy
    retval = sl_WlanPolicySet(SL_POLICY_CONNECTION,
            SL_CONNECTION_POLICY(1, 0, 0, 0, 1),
            NULL,
            0);
    if(retval<0) {RETURN_ERROR(ERROR_UNKNOWN);}

    //delete profiles
    retval = sl_WlanProfileDel(0xFF);
    if(retval<0) {RETURN_ERROR(ERROR_UNKNOWN);}

    //disconnect
    retval = sl_WlanDisconnect();
    if(retval == 0){ //not yet disconnected
        while(IS_CONNECTED(status)){};
    }

    //Enable DHCP client
    retval = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&val);
    if(retval<0) {RETURN_ERROR(ERROR_UNKNOWN);}

    //Disable scan policy
    config = SL_SCAN_POLICY(0);
    retval = sl_WlanPolicySet(SL_POLICY_SCAN, config, NULL, 0);
    if(retval<0) {RETURN_ERROR(ERROR_UNKNOWN);}

    //Set Tx power level for station mode
    //Number between 0-15, as dB offset from max power - 0 will set max power
    val = 0;
    retval = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&val);
    if(retval<0){ RETURN_ERROR(ERROR_UNKNOWN); }

    // Set PM policy to normal
    retval = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    if(retval<0){ RETURN_ERROR(ERROR_UNKNOWN); }

    // Unregister mDNS services
    retval = sl_NetAppMDNSUnRegisterService(0, 0);
    if(retval<0){ RETURN_ERROR(ERROR_UNKNOWN); }

    // Remove  all 64 filters (8*8)
    memset(filter_mask.FilterIdMask, 0xFF, 8);
    retval = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (uint8_t*)&filter_mask,
            sizeof(_WlanRxFilterOperationCommandBuff_t));
    if(retval<0){ RETURN_ERROR(ERROR_UNKNOWN); }

    retval = sl_Stop(SL_STOP_TIMEOUT);
    if(retval<0){ RETURN_ERROR(ERROR_UNKNOWN); }

    //TODO: (re)set state variables

    return RET_SUCCESS;
}

void Task_WifiScan(void* params)
{
    (void)params; //avoid unused error
    long retval;
    unsigned char policy;
    unsigned int policy_len;

    if(WifiStartDefaultSettings() == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);

    retval = sl_Start(0, 0, 0);
    if(retval<0) {error_add(__FILE__, __LINE__, ERROR_UNKNOWN); return;}

    //first, delete current connection policy
    policy = SL_CONNECTION_POLICY(0,0,0,0,0);
    retval = sl_WlanPolicySet(SL_POLICY_CONNECTION, policy, NULL, 0);
    if(retval<0) {error_add(__FILE__,__LINE__,ERROR_UNKNOWN); return;}

    //make scan policy
    policy = SL_SCAN_POLICY(1);
    policy_len = 10; //10 seconds scan time
    retval = sl_WlanPolicySet(SL_POLICY_SCAN, policy, (unsigned char*)&policy_len, sizeof(policy_len));
    if(retval<0) {error_add(__FILE__,__LINE__,ERROR_UNKNOWN); return;}

    //wait for the scan to complete
    const TickType_t delay = 11000 / portTICK_PERIOD_MS;
    vTaskDelay(delay);

    //get the results back
    unsigned char index = 0;
    retval = sl_WlanGetNetworkList(index, (unsigned char)WIFI_NUM_NETWORKS, &(wifi_state.networks[index]));

    //retval holds the number of networks now, and they are saved in the state.

    //disable the scan
    policy = SL_SCAN_POLICY(0);
    retval = sl_WlanPolicySet(SL_POLICY_SCAN, policy, NULL, 0);
    if(retval<0) {error_add(__FILE__,__LINE__,ERROR_UNKNOWN); return;}

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
