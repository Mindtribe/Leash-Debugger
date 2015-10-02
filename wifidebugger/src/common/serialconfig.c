/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "serialconfig.h"

#include <string.h>

#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "wlan.h"
#include "error.h"

#define SC_INPUT_MAXLEN 256
#define SC_CMD_ADD_NETWORK "network"
#define SC_CMD_DELETE_NETWORK "delete"
#define SC_CMD_DELETE_ALL "deleteall"
#define SC_CMD_LIST_NETWORKS "list"
#define SC_CMD_HELP "help"

#define SERIALCONFIG_TASK_STACK_SIZE (2048)
#define SERIALCONFIG_TASK_PRIORITY (2)

#define SSID_MAXLEN 32
#define SECURITY_MAXLEN 5
#define KEY_MAXLEN 63

#define OPEN_STRING "OPEN"
#define WEP_STRING "WEP"
#define WPA_STRING "WPA"

#define NUM_PROFILES_MAX 7

enum command_statemachine{
    COMMAND = 0,
    ADDNEWORK_GET_SSID,
    GET_TYPE,
    GET_KEY,
    DELETENEWORK_GET_SSID,
    CONFIRM_DEL_ALL,
    DELETENETWORK_CONFIRM
};

struct wlan_profile_data_t{
    char name[32];
    unsigned char macaddr[6];
    signed short namelen;
    unsigned long priority;
    SlSecParams_t secparams;
    SlGetSecParamsExt_t extparams;
    unsigned int valid;
};

struct serialconfig_state_t{
    char rxbuf[SC_INPUT_MAXLEN];
    unsigned int rxbuf_i;
    void (*get_char)(char*);
    enum command_statemachine statenum;
    void (*current_operation_callback)(void);
    char temp_ssid[SSID_MAXLEN];
    unsigned int temp_type;
    char temp_key[KEY_MAXLEN];
};

static struct serialconfig_state_t serialconfig_state = {
    .rxbuf = {0},
    .rxbuf_i = 0,
    .get_char = NULL,
    .statenum = COMMAND
};

struct serialconfig_command_t{
    const char* cmdstring;
    const char* helpstring;
    void (*executeCallback)(void);
};

static void sc_cmd_add(void);
static void sc_cmd_del(void);
static void sc_cmd_delall(void);
static void sc_cmd_list(void);
static void sc_cmd_help(void);

static const struct serialconfig_command_t serialconfig_commands[] = {
    {"network", "add a network.", &sc_cmd_add},
    {"delete", "delete a network.", &sc_cmd_del},
    {"deleteall", "delete all networks.", &sc_cmd_delall},
    {"list", "list stored networks.", &sc_cmd_list},
    {"help", "display this help text.", &sc_cmd_help}
};

static void Task_SerialConfig(void* params);
static int get_profiles(struct wlan_profile_data_t *profiles);

void serialconfig_start(void (*get_char)(char*))
{
    serialconfig_state.get_char = get_char;
    serialconfig_state.statenum = COMMAND;

    xTaskCreate(Task_SerialConfig,
            "Serial Config Server",
            SERIALCONFIG_TASK_STACK_SIZE/sizeof(portSTACK_TYPE),
            NULL,
            SERIALCONFIG_TASK_PRIORITY,
            0);
}

void serialconfig_stop(void)
{
    serialconfig_state.get_char = NULL;
}

static int get_profiles(struct wlan_profile_data_t *profiles)
{
    for(int i=0; i<NUM_PROFILES_MAX; i++){
        profiles[i].valid = 0;
        int retval = sl_WlanProfileGet(i,
                (signed char*)profiles[i].name,
                &(profiles[i].namelen),
                profiles[i].macaddr,
                &(profiles[i].secparams),
                &(profiles[i].extparams),
                &(profiles[i].priority));
        if(retval >= 0) {
            profiles[i].name[profiles[i].namelen] = 0;
            profiles[i].valid = 1;
        }
    }

    return RET_SUCCESS;
}

static void sc_cmd_add(void)
{
    unsigned int commit = 0;
    switch(serialconfig_state.statenum){
    case COMMAND:
        serialconfig_state.statenum = ADDNEWORK_GET_SSID;
        LOG(LOG_IMPORTANT, "[CONF] Please enter SSID:");
        break;
    case ADDNEWORK_GET_SSID:
        if(strlen(serialconfig_state.rxbuf) > SSID_MAXLEN){
            LOG(LOG_IMPORTANT, "[CONF] SSID too long.");
            serialconfig_state.statenum = COMMAND;
        }
        strcpy(serialconfig_state.temp_ssid, serialconfig_state.rxbuf);
        LOG(LOG_IMPORTANT, "[CONF] Please enter security type (OPEN/WEP/WPA):");
        serialconfig_state.statenum = GET_TYPE;
        break;
    case GET_TYPE:
        if(strcmp(serialconfig_state.rxbuf, OPEN_STRING) == 0){
            serialconfig_state.temp_type = SL_SEC_TYPE_OPEN;
            serialconfig_state.statenum = COMMAND;
            commit = 1;
        }
        else if(strcmp(serialconfig_state.rxbuf, WPA_STRING) == 0){
            serialconfig_state.temp_type = SL_SEC_TYPE_WPA_WPA2;
        }
        else if(strcmp(serialconfig_state.rxbuf, WEP_STRING) == 0){
            serialconfig_state.temp_type = SL_SEC_TYPE_WEP;
        }
        else{
            LOG(LOG_IMPORTANT, "[CONF] Invalid security type.");
            serialconfig_state.statenum = COMMAND;
            break;
        }

        LOG(LOG_IMPORTANT, "[CONF] Please enter security key:")
        serialconfig_state.statenum = GET_KEY;
        break;
    case GET_KEY:
        if(strlen(serialconfig_state.rxbuf) > KEY_MAXLEN){
            LOG(LOG_IMPORTANT, "[CONF] Key too long.");
            serialconfig_state.statenum = COMMAND;
            break;
        }
        strcpy(serialconfig_state.temp_key, serialconfig_state.rxbuf);
        serialconfig_state.statenum = COMMAND;
        commit = 1;
        break;
    default:
        serialconfig_state.statenum = COMMAND;
        return;
    }
    if(commit){
        LOG(LOG_IMPORTANT, "[CONF] Adding network '%s'...", serialconfig_state.temp_ssid);

        struct wlan_profile_data_t profiles[NUM_PROFILES_MAX];
        if(get_profiles(profiles) == RET_FAILURE) { WAIT_ERROR(ERROR_UNKNOWN); }

        short delete_profile = -1;
        int add_profile = 0;
        int ready_commit = 0;

        for(short i=0; i<NUM_PROFILES_MAX; i++){
            if(strcmp(serialconfig_state.temp_ssid, profiles[i].name) == 0){
                LOG(LOG_IMPORTANT, "[COMP] '%s' found in profiles, replacing it...", profiles[i].name);
                delete_profile = i;
                ready_commit = 1;
                add_profile = 1;
                break;
            }
        }
        if(!ready_commit){
            for(int i=0; i<NUM_PROFILES_MAX; i++){
                if(!profiles[i].valid){
                    add_profile = 1;
                    ready_commit = 1;
                }
            }
        }
        if(!ready_commit){
            LOG(LOG_IMPORTANT, "[COMP] Failed: no room in profiles list.");
        }
        else{
            if(delete_profile >= 0){
                if(sl_WlanProfileDel(delete_profile) < 0){
                    LOG(LOG_IMPORTANT, "[COMP] Delete failed - unknown error.");
                }
            }
            if(add_profile){
                SlSecParams_t secparams = {0};
                secparams.Type = serialconfig_state.temp_type;
                secparams.KeyLen = strlen(serialconfig_state.temp_key);
                secparams.Key = (signed char*)serialconfig_state.temp_key;
                int retval = sl_WlanProfileAdd(
                        (signed char*)serialconfig_state.temp_ssid,
                        (short)strlen(serialconfig_state.temp_ssid),
                        NULL,
                        &secparams,
                        NULL,
                        0,
                        0);
                if(retval<0){  LOG(LOG_IMPORTANT, "[COMP] Add failed - unknown error."); }
                else{ LOG(LOG_IMPORTANT, "[COMP] '%s' added.", serialconfig_state.temp_ssid);}
            }
        }
    }
}

static void sc_cmd_del(void)
{
    unsigned int delete = 0;
    switch(serialconfig_state.statenum){
    case COMMAND:
        serialconfig_state.statenum = DELETENEWORK_GET_SSID;
        LOG(LOG_IMPORTANT, "[CONF] Please enter SSID:");
        break;
    case DELETENEWORK_GET_SSID:
        if(strlen(serialconfig_state.rxbuf) > SSID_MAXLEN){
            LOG(LOG_IMPORTANT, "[CONF] SSID too long.");
            serialconfig_state.statenum = COMMAND;
        }
        strcpy(serialconfig_state.temp_ssid, serialconfig_state.rxbuf);
        LOG(LOG_IMPORTANT, "[CONF] Delete '%s'? (Y/N)", serialconfig_state.temp_ssid);
        serialconfig_state.statenum = DELETENETWORK_CONFIRM;
        break;
    case DELETENETWORK_CONFIRM:
        if(strcmp(serialconfig_state.rxbuf, "Y") == 0){
            delete = 1;
            serialconfig_state.statenum = COMMAND;
        }
        else { LOG(LOG_IMPORTANT, "[CONF] Cancelled."); }
        break;
    default:
        serialconfig_state.statenum = COMMAND;
        return;
    }
    if(delete){
        LOG(LOG_IMPORTANT, "[CONF] Deleting network '%s'...", serialconfig_state.temp_ssid);

        struct wlan_profile_data_t profiles[NUM_PROFILES_MAX];
        if(get_profiles(profiles) == RET_FAILURE) { WAIT_ERROR(ERROR_UNKNOWN); }
        unsigned int found = 0;

        for(int i=0; i<NUM_PROFILES_MAX; i++){
            if(strcmp(serialconfig_state.temp_ssid, profiles[i].name) == 0){
                found = 1;
                if(sl_WlanProfileDel(i) < 0){
                    LOG(LOG_IMPORTANT, "[COMP] Delete failed - unknown error.");
                }
                else{ LOG(LOG_IMPORTANT, "[COMP] Deleted '%s'.", serialconfig_state.temp_ssid); }
            }
        }
        if(!found){ LOG(LOG_IMPORTANT, "[COMP] Delete failed - not found."); }
    }
}

static void sc_cmd_delall(void)
{
    unsigned int deleteall = 0;
    switch(serialconfig_state.statenum){
    case COMMAND:
        serialconfig_state.statenum = CONFIRM_DEL_ALL;
        LOG(LOG_IMPORTANT, "[CONF] Delete ALL networks? (Y/N)");
        break;
    case CONFIRM_DEL_ALL:
        if(strcmp(serialconfig_state.rxbuf, "Y") == 0){
            deleteall = 1;
            serialconfig_state.statenum = COMMAND;
        }
        else { LOG(LOG_IMPORTANT, "[CONF] Cancelled."); }
        break;
    default:
        serialconfig_state.statenum = COMMAND;
        return;
    }
    if(deleteall){
        if(sl_WlanProfileDel(255) < 0){
            LOG(LOG_IMPORTANT, "[COMP] Delete failed - unknown error.");
        }
        else { LOG(LOG_IMPORTANT, "[COMP] Deleted all networks."); }
    }
}

static void sc_cmd_list(void)
{
    struct wlan_profile_data_t profiles[NUM_PROFILES_MAX];
    switch(serialconfig_state.statenum){
    case COMMAND:
        if(get_profiles(profiles) == RET_FAILURE) { LOG(LOG_IMPORTANT, "[CONF] List retrieval failed!");}
        else{
            LOG(LOG_IMPORTANT, "[CONF] Networks stored:");
            for(int i=0; i<NUM_PROFILES_MAX; i++){
                if(profiles[i].valid){ LOG(LOG_IMPORTANT, "[CONF] '%s'", profiles[i].name); }
            }
        }
        break;
    default:
        serialconfig_state.statenum = COMMAND;
        return;
    }
}

static void sc_cmd_help(void)
{
    switch(serialconfig_state.statenum){
    case COMMAND:
        LOG(LOG_IMPORTANT, "[CONF] Supported commands:");
        for(unsigned int i=0; i<(sizeof(serialconfig_commands)/sizeof(struct serialconfig_command_t)); i++){
            LOG(LOG_IMPORTANT, "[CONF] %s: %s", serialconfig_commands[i].cmdstring, serialconfig_commands[i].helpstring);
        }
        break;
    default:
        serialconfig_state.statenum = COMMAND;
        return;
    }
}

static void HandleMessage(void)
{
    int cmd_found = 0;
    switch(serialconfig_state.statenum){
    case COMMAND:
        for(unsigned int i=0; (cmd_found == 0) && (i<(sizeof(serialconfig_commands)/sizeof(struct serialconfig_command_t))); i++){
            if(strcmp(serialconfig_state.rxbuf, serialconfig_commands[i].cmdstring) == 0){
                serialconfig_commands[i].executeCallback();
                serialconfig_state.current_operation_callback = serialconfig_commands[i].executeCallback;
                cmd_found = 1;
            }
        }
        if(cmd_found == 0){
            LOG(LOG_IMPORTANT, "[CONF] Invalid command.");
        }
        break;
    default:
        serialconfig_state.current_operation_callback();
        break;
    }
    return;
}

static void Task_SerialConfig(void* params)
{
    (void)params;

    if(serialconfig_state.get_char == NULL){ vTaskDelete(NULL); }

    for(;;){
        serialconfig_state.get_char(&(serialconfig_state.rxbuf[serialconfig_state.rxbuf_i++]));
        if(serialconfig_state.rxbuf[serialconfig_state.rxbuf_i-1] == '\n'){
            serialconfig_state.rxbuf[serialconfig_state.rxbuf_i-1] = 0;
            HandleMessage();
            serialconfig_state.rxbuf_i = 0;
        }
        else if((serialconfig_state.rxbuf_i-1) >= SC_INPUT_MAXLEN){
            LOG(LOG_IMPORTANT, "[CONF] Input too long, reset.");
            serialconfig_state.rxbuf_i = 0;
        }
    }
}
