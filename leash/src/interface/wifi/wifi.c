/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "wifi.h"

//FreeRTOS includes
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

//TI OS abstraction layer
#include "osi.h"

//SimpleLink includes
#include "simplelink.h"

//project includes
#include "error.h"
#include "simplelink_defs.h"
#include "log.h"
#include "serialsock.h"
#include "led.h"
#include "ui.h"

#ifndef sl_WlanEvtHdlr
#error SimpleLink event handler not defined!
#endif

#define SPAWNTASK_STACK_SIZE (2048)
#define SPAWNTASK_PRIORITY (9)

#define WIFI_AP_SSID "WIFIDEBUGGER"
#define WIFI_SCAN_TIME_S 5

//SimpleLink spawn message type: specifies
//a callback and parameter pointer.
typedef struct
{
    void (*pEntry)(void*);
    void* pValue;
}tSLSpawnMessage;

struct wifi_state_t wifi_state ={
    .version = {
        .NwpVersion = {0}
    },
    .status = 0,
    .client_IP = 0,
    .self_IP = 0,
    .startAP = 0
};

static int WifiSetModeAP();
static int WifiConnectSTA();

int WifiInit(unsigned int startAP)
{
    wifi_state.startAP = startAP;
    if(VStartSimpleLinkSpawnTask(SPAWNTASK_PRIORITY) < 0) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}

int WifiDeinit(void)
{
    if(WifiDeleteSpawnTask() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    return RET_SUCCESS;
}

int WifiDeleteSpawnTask(void)
{
    VDeleteSimpleLinkSpawnTask();
    return RET_SUCCESS;
}

static void WifiTaskEndCallback(void (*taskAddr)(void*))
{
    if(taskAddr == &Task_WifiScan){
        xTaskCreate(Task_WifiSTA,
                "WiFi Station",
                WIFI_TASK_STACK_SIZE/sizeof(portSTACK_TYPE),
                0,
                WIFI_TASK_PRIORITY,
                0);
    }

    return;
}

static int WifiDefaultSettings(void)
{
    long retval = -1;
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
            while(!IS_IP_ACQUIRED(wifi_state.status)){
#ifndef SL_PLATFORM_MULTI_THREADED
                _SlNonOsMainLoopTask();
#endif
            }
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
    if(retval<0) {RETURN_ERROR(retval);}

    //default connection policy
    retval = sl_WlanPolicySet(SL_POLICY_CONNECTION,
            SL_CONNECTION_POLICY(1, 0, 0, 0, 0),
            NULL,
            0);
    if(retval<0) {RETURN_ERROR(retval);}

    //disconnect
    retval = sl_WlanDisconnect();
    if(retval == 0){ //not yet disconnected
        while(IS_CONNECTED(wifi_state.status)){
#ifndef SL_PLATFORM_MULTI_THREADED
            _SlNonOsMainLoopTask();
#endif
        }
    }

    //Enable DHCP client
    retval = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&val);
    if(retval<0) {RETURN_ERROR(retval);}

    //Disable scan policy
    config = SL_SCAN_POLICY(0);
    retval = sl_WlanPolicySet(SL_POLICY_SCAN, config, NULL, 0);
    if(retval<0) {RETURN_ERROR(retval);}

    //Set Tx power level for station mode
    //Number between 0-15, as dB offset from max power - 0 will set max power
    val = 0;
    retval = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&val);
    if(retval<0){ RETURN_ERROR(retval); }

    // Set PM policy to normal
    retval = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    if(retval<0){ RETURN_ERROR(retval); }

    // Unregister mDNS services
    retval = sl_NetAppMDNSUnRegisterService(0, 0);
    if(retval<0){ RETURN_ERROR(retval); }

    //Set mDNS device hostname
    char hostname[64];
    char macstring[20];
    unsigned char mac[SL_MAC_ADDR_LEN];
    unsigned char maclen = SL_MAC_ADDR_LEN;
    retval = sl_NetCfgGet(SL_MAC_ADDRESS_GET,
            NULL,
            &maclen,
            mac);
    if(retval < 0) { RETURN_ERROR(retval); }
    snprintf(macstring, 20,  "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(hostname, 64,  "WiFiDebugger%s", macstring);
    retval = sl_NetAppSet (SL_NET_APP_DEVICE_CONFIG_ID,
            NETAPP_SET_GET_DEV_CONF_OPT_DEVICE_URN,
            strlen((const char *)hostname),
            (unsigned char *) hostname);
    if(retval<0) {RETURN_ERROR(retval);}

    // Remove  all 64 filters (8*8)
    memset(filter_mask.FilterIdMask, 0xFF, 8);
    retval = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (uint8_t*)&filter_mask,
            sizeof(_WlanRxFilterOperationCommandBuff_t));
    if(retval<0){ RETURN_ERROR(retval); }

    retval = sl_Stop(SL_STOP_TIMEOUT);
    if(retval < 0) {RETURN_ERROR(ERROR_UNKNOWN);}

    wifi_state.status = 0;

    return RET_SUCCESS;
}

static int WifiSetModeAP()
{
    long retval;

    if((retval = sl_WlanSetMode(ROLE_AP)) < 0) {RETURN_ERROR(ERROR_UNKNOWN);}

    if((retval = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, strlen(WIFI_AP_SSID), (unsigned char*)WIFI_AP_SSID)) < 0){
        RETURN_ERROR((int)retval);
    }

    //restart
    if((retval = sl_Stop(SL_STOP_TIMEOUT)) < 0) {RETURN_ERROR(ERROR_UNKNOWN);}
    CLR_STATUS_BIT_ALL(wifi_state.status);
    return sl_Start(0,0,0);
}

static int WifiConnectSTA()
{
    LOG(LOG_IMPORTANT ,"Waiting for auto connect...");

    while((!IS_CONNECTED(wifi_state.status)) || (!IS_IP_ACQUIRED(wifi_state.status))){
#ifndef SL_PLATFORM_MULTI_THREADED
        _SlNonOsMainLoopTask();
#endif
    }

    return RET_SUCCESS;
}

void Task_Wifi(void* params)
{
    (void)params; //avoid unused error

    LOG(LOG_VERBOSE, "WiFi Task started.");

    if(wifi_state.startAP){
        //start the access point.
        xTaskCreate(Task_WifiAP,
                "WiFi AP",
                WIFI_TASK_STACK_SIZE/sizeof(portSTACK_TYPE),
                0,
                WIFI_TASK_PRIORITY,
                0);
    }
    else{
        xTaskCreate(Task_WifiSTA,
                "WiFi Station",
                WIFI_TASK_STACK_SIZE/sizeof(portSTACK_TYPE),
                0,
                WIFI_TASK_PRIORITY,
                0);
    }

    WifiTaskEndCallback(&Task_Wifi);
    vTaskDelete(NULL);
    return;
}

void Task_WifiScan(void* params)
{
    (void)params; //avoid unused error
    long retval;
    unsigned char policy;
    unsigned int policy_len;

    LOG(LOG_VERBOSE, "Starting WiFi network scan...");

    SetLEDBlink(LED_WIFI, LED_BLINK_PATTERN_WIFI_SCANNING);

    if(WifiDefaultSettings() == RET_FAILURE) {goto error;}

    retval = sl_Start(0,0,0);
    if(retval<0)  {goto error;}

    //first, delete current connection policy
    policy = SL_CONNECTION_POLICY(0,0,0,0,0);
    retval = sl_WlanPolicySet(SL_POLICY_CONNECTION, policy, NULL, 0);
    if(retval<0)  {goto error;}

    //make scan policy
    policy = SL_SCAN_POLICY(1);
    policy_len = WIFI_SCAN_TIME_S;
    retval = sl_WlanPolicySet(SL_POLICY_SCAN, policy, (unsigned char*)&policy_len, sizeof(policy_len));
    if(retval<0)  {goto error;}

    //wait for the scan to complete
    const TickType_t delay = (1100*WIFI_SCAN_TIME_S) / portTICK_PERIOD_MS;
    vTaskDelay(delay);

    //get the results back
    unsigned char index = 0;
    retval = sl_WlanGetNetworkList(index, (unsigned char)WIFI_NUM_NETWORKS, &(wifi_state.networks[index]));

    //retval holds the number of networks now, and they are saved in the state.

    //disable the scan
    policy = SL_SCAN_POLICY(0);
    retval = sl_WlanPolicySet(SL_POLICY_SCAN, policy, NULL, 0);
    if(retval<0)  {goto error;}

    //disable SimpleLink altogether
    retval = sl_Stop(SL_STOP_TIMEOUT);
    if(retval<0)  {goto error;}

    LOG(LOG_VERBOSE, "WiFi network scan complete.");

    ClearLED(LED_WIFI);

    //exit (delete this task)
    WifiTaskEndCallback(&Task_WifiScan);
    vTaskDelete(NULL);

    return;

    error:
    SetLEDBlink(LED_WIFI, LED_BLINK_PATTERN_WIFI_FAILED);
    TASK_RETURN_ERROR(ERROR_UNKNOWN);
    return;
}

void Task_WifiAP(void* params)
{
    (void)params; //avoid unused warning
    long retval;

    LOG(LOG_IMPORTANT, "Starting WiFi AP Mode.");
    SetLEDBlink(LED_WIFI, LED_BLINK_PATTERN_WIFI_CONNECTING);

    if(WifiDefaultSettings() == RET_FAILURE)  {goto error;}

    if((retval = sl_Start(0,0,0)) < 0)  {goto error;}

    if(retval != ROLE_AP){
        if(WifiSetModeAP() != ROLE_AP){
            sl_Stop(SL_STOP_TIMEOUT);
            goto error;
        }
    }

    //Wait for an IP address to be acquired
    while(!IS_IP_ACQUIRED(wifi_state.status)){};

    SetLEDBlink(LED_WIFI, LED_BLINK_PATTERN_WIFI_AP);
    LOG(LOG_IMPORTANT, "AP Started - Ready for client.");

    //start socket handler.
    xTaskCreate(Task_SocketHandler,
            "Socket Handler",
            SOCKET_TASK_STACK_SIZE/sizeof(portSTACK_TYPE),
            0,
            SOCKET_TASK_PRIORITY,
            0);

    //exit (delete this task)
    WifiTaskEndCallback(&Task_WifiAP);
    vTaskDelete(NULL);

    error:
    SetLED(LED_WIFI, LED_BLINK_PATTERN_WIFI_FAILED);
    TASK_RETURN_ERROR(ERROR_UNKNOWN);
    return;

    return;
}

void Task_WifiSTA(void* params)
{
    (void)params; //avoid unused warning
    long retval;

    LOG(LOG_IMPORTANT, "Starting WiFi Station Mode.");
    SetLEDBlink(LED_WIFI, LED_BLINK_PATTERN_WIFI_CONNECTING);

    if(WifiDefaultSettings() == RET_FAILURE) {goto error;}

    if((retval = sl_Start(0,0,0)) < 0) {goto error;}
    if(retval != ROLE_STA) {goto error;}

    LOG(LOG_VERBOSE, "WiFi set to station mode.");

    if(WifiConnectSTA() == RET_FAILURE) {goto error;}

    SetLEDBlink(LED_WIFI, LED_BLINK_PATTERN_WIFI_CONNECTED);

    //start socket handler.
    xTaskCreate(Task_SocketHandler,
            "Socket Handler",
            SOCKET_TASK_STACK_SIZE/sizeof(portSTACK_TYPE),
            0,
            SOCKET_TASK_PRIORITY,
            0);

    //exit (delete this task)
    WifiTaskEndCallback(&Task_WifiAP);
    vTaskDelete(NULL);

    return;

    error:
    SetLED(LED_WIFI, LED_BLINK_PATTERN_WIFI_FAILED);
    TASK_RETURN_ERROR(ERROR_UNKNOWN);
    return;
}
