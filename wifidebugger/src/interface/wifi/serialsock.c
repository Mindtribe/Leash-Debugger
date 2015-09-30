/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "serialsock.h"
#include "wifi.h"
#include "simplelink_defs.h"
#include "netcfg.h"
#include "netapp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "serialconfig.h"
#include "socket_defs.h"

//status bits
#define SOCKET_STARTED 1
#define SOCKET_CONNECTED 2

#define IS_SOCK_STARTED(X) (socket_state[(X)].status & SOCKET_STARTED)
#define IS_SOCK_CONNECTED(X) (socket_state[(X)].status & SOCKET_CONNECTED)

#define SOCKET_TXBUF_SIZE 128
#define SOCKET_RXBUF_SIZE 128

#define MDNS_SERVICE_NAME_MAXLEN 128
#define MDNS_SERVICE_TTL 2000

//Note: be aware that arguments to this macro get evaluated twice.
//(so no MIN(x++, y++)!)
#define MIN(X, Y) ((X) < (Y)) ? (X) : (Y)

struct socket_state_t{
    unsigned int status;
    int id;
    SlSockAddrIn_t addr_local;
    SlSockAddrIn_t addr_remote;
    SemaphoreHandle_t buf_access;
    SemaphoreHandle_t tx_buf_full;
    SemaphoreHandle_t rx_buf_empty;
    char tx_buf[SOCKET_TXBUF_SIZE];
    unsigned int tx_buf_i_in;
    unsigned int tx_buf_i_out;
    unsigned int tx_buf_occupancy;
    char rx_buf[SOCKET_RXBUF_SIZE];
    unsigned int rx_buf_i_in;
    unsigned int rx_buf_i_out;
    unsigned int rx_buf_occupancy;
    unsigned int initialized;
};

const char* buf_access_mutex_names[3] = {
    "S0 Buf Access Mutex",
    "S1 Buf Access Mutex",
    "S2 Buf Access Mutex"
};
const char* tx_buf_full_names[3] = {
    "S0 Tx Full Mutex",
    "S1 Tx Full Mutex",
    "S2 Tx Full Mutex"
};
const char* rx_buf_empty_names[3] = {
    "S0 Rx Empty Mutex",
    "S1 Rx Empty Mutex",
    "S2 Rx Empty Mutex"
};

struct socket_state_t socket_state[NUM_SOCKETS] = {{0}};

void LogSLError(int code){
    switch(code){
    case SL_ENSOCK:
        LOG(LOG_IMPORTANT, "[SLERR] Maximum socket amount reached.");
        break;
    case SL_ENOMEM:
        LOG(LOG_IMPORTANT, "[SLERR] Out of memory.");
        break;
    case SL_ENETUNREACH:
        LOG(LOG_IMPORTANT, "[SLERR] Network unreachable.");
        break;
    case SL_ENOBUFS:
        LOG(LOG_IMPORTANT, "[SLERR] No buffers available.");
        break;
    case SL_EISCONN:
        LOG(LOG_IMPORTANT, "[SLERR] Already connected.");
        break;
    case SL_ENOTCONN:
        LOG(LOG_IMPORTANT, "[SLERR] Not connected.");
        break;
    case SL_ETIMEDOUT:
        LOG(LOG_IMPORTANT, "[SLERR] Timeout.");
        break;
    case SL_ECONNREFUSED:
        LOG(LOG_IMPORTANT, "[SLERR] Connection refused.");
        break;
    default:
        break;
    }
}

static int ListenSerialSock(unsigned int slot);

void InitSockets(void)
{
    for(int i=0; i<NUM_SOCKETS; i++){
        if(!socket_state[i].initialized){
            if(socket_state[i].buf_access == NULL){
                socket_state[i].buf_access = xSemaphoreCreateMutex();
                vQueueAddToRegistry(socket_state[i].buf_access, buf_access_mutex_names[i]);
                socket_state[i].tx_buf_full = xSemaphoreCreateMutex();
                vQueueAddToRegistry(socket_state[i].tx_buf_full, tx_buf_full_names[i]);
                socket_state[i].rx_buf_empty = xSemaphoreCreateMutex();
                vQueueAddToRegistry(socket_state[i].rx_buf_empty, rx_buf_empty_names[i]);

                //take send/receive semaphores so that other threads cannot send/receive
                //before sockets connect
                xSemaphoreTake(socket_state[i].tx_buf_full, portMAX_DELAY);
                xSemaphoreTake(socket_state[i].rx_buf_empty, portMAX_DELAY);
            }
            socket_state[i].initialized = 1;
        }
    }
}

static int RegisterSocketServices(void)
{
    int retval;
    char servicename[MDNS_SERVICE_NAME_MAXLEN];

    LOG(LOG_IMPORTANT, "[WIFI] Registering services on mDNS...");

    //retreive the device mac address
    char macstring[20];
    unsigned char mac[SL_MAC_ADDR_LEN];
    unsigned char maclen = SL_MAC_ADDR_LEN;
    retval = sl_NetCfgGet(SL_MAC_ADDRESS_GET,
            NULL,
            &maclen,
            mac);
    if(retval < 0) { RETURN_ERROR(retval); }
    sprintf(macstring, "[%02X:%02X:%02X:%02X:%02X:%02X]", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    for(int i=0; i<NUM_SOCKETS; i++){
        sprintf(servicename, "%s - %s", macstring, socket_mdns_names_fixedpart[i]);
        retval = sl_NetAppMDNSRegisterService((signed char*)servicename,
                (unsigned char)strlen(servicename),
                (signed char*)socket_mdns_descriptions[i],
                (unsigned char)strlen(socket_mdns_descriptions[i]),
                socket_ports[i],MDNS_SERVICE_TTL,1);
        if(retval < 0) { RETURN_ERROR(retval); }
    }

    LOG(LOG_IMPORTANT, "[WIFI] Services registered.");

    return RET_SUCCESS;
}

int StartSockets(void)
{
    for(int i=0; i<NUM_SOCKETS; i++){
        if(StartSerialSock(socket_ports[i], i) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    }

    if(RegisterSocketServices() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    return RET_SUCCESS;
}

void StopSockets(void)
{
    for(int i = 0; i<NUM_SOCKETS; i++){
        if(IS_SOCK_STARTED(i)){
            sl_Close(socket_state[i].id);
        }
        socket_state[i].status = 0;
    }

    return;
}

int SocketPutChar(char c, unsigned int socket_slot)
{
    for(;;){
        xSemaphoreTake(socket_state[socket_slot].buf_access, portMAX_DELAY);
        if(socket_state[socket_slot].tx_buf_occupancy < SOCKET_TXBUF_SIZE) {break;}
        xSemaphoreGive(socket_state[socket_slot].buf_access);
        UpdateSockets();
    }

    socket_state[socket_slot].tx_buf[(socket_state[socket_slot].tx_buf_i_in++)%SOCKET_TXBUF_SIZE] = c;
    socket_state[socket_slot].tx_buf_occupancy++;
    xSemaphoreGive(socket_state[socket_slot].buf_access);

    return RET_SUCCESS;
}

int SocketGetChar(char *c, unsigned int socket_slot)
{
    for(;;){
        xSemaphoreTake(socket_state[socket_slot].buf_access, portMAX_DELAY);
        if(socket_state[socket_slot].rx_buf_occupancy > 0) {break;}
        xSemaphoreGive(socket_state[socket_slot].buf_access);
        UpdateSockets();
    }

    *c = socket_state[socket_slot].rx_buf[(socket_state[socket_slot].rx_buf_i_out++)%SOCKET_RXBUF_SIZE];
    socket_state[socket_slot].rx_buf_occupancy--;
    xSemaphoreGive(socket_state[socket_slot].buf_access);

    return RET_SUCCESS;
}

int TS_SocketPutChar(char c, unsigned int socket_slot)
{
    xSemaphoreTake(socket_state[socket_slot].tx_buf_full, portMAX_DELAY);
    xSemaphoreTake(socket_state[socket_slot].buf_access, portMAX_DELAY);
    socket_state[socket_slot].tx_buf[(socket_state[socket_slot].tx_buf_i_in++)%SOCKET_TXBUF_SIZE] = c;
    socket_state[socket_slot].tx_buf_occupancy++;
    xSemaphoreGive(socket_state[socket_slot].buf_access);
    //block all senders until space frees up
    if(socket_state[socket_slot].tx_buf_occupancy < SOCKET_TXBUF_SIZE){
        xSemaphoreGive(socket_state[socket_slot].tx_buf_full);
    }
    return RET_SUCCESS;
}

void TS_LogSocketPutChar(char c)
{
    TS_SocketPutChar(c,SOCKET_LOG);
}

void TS_TargetSocketPutChar(char c)
{
    TS_SocketPutChar(c,SOCKET_TARGET);
}

void TS_GDBSocketPutChar(char c)
{
    TS_SocketPutChar(c,SOCKET_GDB);
}

int TS_SocketGetChar(char *c, unsigned int socket_slot)
{
    xSemaphoreTake(socket_state[socket_slot].rx_buf_empty, portMAX_DELAY);
    xSemaphoreTake(socket_state[socket_slot].buf_access, portMAX_DELAY);
    *c = socket_state[socket_slot].rx_buf[(socket_state[socket_slot].rx_buf_i_out++)%SOCKET_RXBUF_SIZE];
    socket_state[socket_slot].rx_buf_occupancy--;
    xSemaphoreGive(socket_state[socket_slot].buf_access);
    //block all receivers until characters appear
    if(socket_state[socket_slot].rx_buf_occupancy > 0){
        xSemaphoreGive(socket_state[socket_slot].rx_buf_empty);
    }
    return RET_SUCCESS;
}

void TS_LogSocketGetChar(char *c)
{
    TS_SocketGetChar(c,SOCKET_LOG);
}

void TS_TargetSocketGetChar(char *c)
{
    TS_SocketGetChar(c,SOCKET_TARGET);
}

void TS_GDBSocketGetChar(char *c)
{
    TS_SocketGetChar(c,SOCKET_GDB);
}

int SocketRXCharAvailable(unsigned int socket_slot)
{
    int occupancy;
    xSemaphoreTake(socket_state[socket_slot].buf_access, portMAX_DELAY);
    occupancy = socket_state[socket_slot].rx_buf_occupancy;
    xSemaphoreGive(socket_state[socket_slot].buf_access);

    return occupancy;
}

int SocketTXSpaceAvailable(unsigned int socket_slot)
{
    int space;
    xSemaphoreTake(socket_state[socket_slot].buf_access, portMAX_DELAY);
    space = SOCKET_TXBUF_SIZE - socket_state[socket_slot].tx_buf_occupancy;
    xSemaphoreGive(socket_state[socket_slot].buf_access);

    return space;
}

int TS_LogSocketRXCharAvailable(void)
{
    return TS_SocketRXCharAvailable(SOCKET_LOG);
}

int TS_GDBSocketRXCharAvailable(void)
{
    return TS_SocketRXCharAvailable(SOCKET_GDB);
}

int TS_TargetSocketRXCharAvailable(void)
{
    return TS_SocketRXCharAvailable(SOCKET_TARGET);
}

int TS_LogSocketTXSpaceAvailable(void)
{
    return TS_SocketRXCharAvailable(SOCKET_LOG);
}

int TS_GDBSocketTXSpaceAvailable(void)
{
    return TS_SocketRXCharAvailable(SOCKET_GDB);
}

int TS_TargetSocketTXSpaceAvailable(void)
{
    return TS_SocketRXCharAvailable(SOCKET_TARGET);
}

int TS_SocketRXCharAvailable(unsigned int socket_slot)
{
    int occupancy;
    xSemaphoreTake(socket_state[socket_slot].buf_access, portMAX_DELAY);
    occupancy = socket_state[socket_slot].rx_buf_occupancy;
    xSemaphoreGive(socket_state[socket_slot].buf_access);

    return occupancy;
}

int TS_SocketTXSpaceAvailable(unsigned int socket_slot)
{
    int space;
    xSemaphoreTake(socket_state[socket_slot].buf_access, portMAX_DELAY);
    space = SOCKET_TXBUF_SIZE - socket_state[socket_slot].tx_buf_occupancy;
    xSemaphoreGive(socket_state[socket_slot].buf_access);

    return space;
}

int UpdateSockets(void)
{
    int retval;

    for(int i=0; i<NUM_SOCKETS; i++){
        if(!GetSockConnected(i)){
            if(SockAccept(i) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        }
        else{
            xSemaphoreTake(socket_state[i].buf_access, portMAX_DELAY);

            //Update TX
            if(socket_state[i].tx_buf_occupancy){
                retval = sl_Send(socket_state[i].id,
                        &(socket_state[i].tx_buf[socket_state[i].tx_buf_i_out % SOCKET_RXBUF_SIZE]),
                        MIN(socket_state[i].tx_buf_occupancy,
                                SOCKET_TXBUF_SIZE - (socket_state[i].tx_buf_i_out % SOCKET_RXBUF_SIZE)), //Prevent overflow at wraparound point
                                0);
                if(retval<0) {
                    LogSLError(retval);
                    xSemaphoreGive(socket_state[i].buf_access); RETURN_ERROR(retval);
                }
                else{
                    socket_state[i].tx_buf_occupancy -= retval;
                    socket_state[i].tx_buf_i_out += retval;
                }
            }
            if((xSemaphoreTake(socket_state[i].tx_buf_full, 0) == pdFALSE) //poll semaphore
                    && (socket_state[i].tx_buf_occupancy<SOCKET_TXBUF_SIZE)){
                xSemaphoreGive(socket_state[i].tx_buf_full); //unblock threads waiting for space
            }

            //Update RX
            retval = sl_Recv(socket_state[i].id,
                    &(socket_state[i].rx_buf[socket_state[i].rx_buf_i_in % SOCKET_RXBUF_SIZE]),
                    MIN((SOCKET_RXBUF_SIZE - socket_state[i].rx_buf_occupancy),
                            SOCKET_RXBUF_SIZE - (socket_state[i].rx_buf_i_in % SOCKET_RXBUF_SIZE)), //Prevent overflow at wraparound point
                            0);
            if(retval > 0){
                socket_state[i].rx_buf_i_in += retval;
                socket_state[i].rx_buf_occupancy += retval;
                if(xSemaphoreTake(socket_state[i].rx_buf_empty, 0) == pdFALSE) { //poll semaphore
                    xSemaphoreGive(socket_state[i].rx_buf_empty); //unblock threads waiting for characters
                }
            }
            else if(retval==0){
                socket_state[i].status = SOCKET_STARTED;
                LOG(LOG_VERBOSE, "Socket %d: client disconnected/broken.", i);
                ListenSerialSock(i);
                while(1)
                    ;
            }
            else if((retval<0) && (retval!=SL_EAGAIN)) {
                LogSLError(retval);
                xSemaphoreGive(socket_state[i].buf_access); RETURN_ERROR(retval);
            }


            xSemaphoreGive(socket_state[i].buf_access);
        }
    }

    return RET_SUCCESS;
}

static int ListenSerialSock(unsigned int slot)
{
    long nonblock = 1;

    int retval = sl_Listen(socket_state[slot].id, 0);
    if(retval < 0){
        sl_Close(socket_state[slot].id);
        RETURN_ERROR(retval);
    }

    //set to non-blocking
    retval = sl_SetSockOpt(socket_state[slot].id, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &nonblock, sizeof(nonblock));
    if(retval < 0){
        sl_Close(socket_state[slot].id);
        RETURN_ERROR(retval);
    }

    return RET_SUCCESS;
}

int StartSerialSock(unsigned short port, unsigned int slot)
{
    if(!IS_IP_ACQUIRED(wifi_state.status)) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(IS_SOCK_STARTED(slot)) {RETURN_ERROR(ERROR_UNKNOWN);}

    LOG(LOG_VERBOSE, "Starting socket %d on port %d...", slot, port);

    int retval;

    socket_state[slot].addr_local.sin_family = SL_AF_INET;
    socket_state[slot].addr_local.sin_port = sl_Htons(port);
    socket_state[slot].addr_local.sin_addr.s_addr = 0;

    socket_state[slot].id = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0);
    if(socket_state[slot].id < 0) {RETURN_ERROR(socket_state[slot].id);}

    retval = sl_Bind(socket_state[slot].id, (SlSockAddr_t*)&(socket_state[slot].addr_local), sizeof(SlSockAddrIn_t));
    if(retval < 0){
        sl_Close(socket_state[slot].id);
        RETURN_ERROR(retval);
    }

    retval = ListenSerialSock(slot);

    LOG(LOG_VERBOSE, "Socket started.");

    socket_state[slot].status = SOCKET_STARTED;
    return RET_SUCCESS;
}

int SockAccept(unsigned int slot)
{
    if(!IS_SOCK_STARTED(slot) || IS_SOCK_CONNECTED(slot)) {RETURN_ERROR(ERROR_UNKNOWN)};

    int addrsize = sizeof(SlSockAddrIn_t);
    int newid = sl_Accept(socket_state[slot].id, (SlSockAddr_t*)&(socket_state[slot].addr_local), (SlSocklen_t *) &addrsize);
    if((newid != SL_EAGAIN) && (newid>=0)){
        LOG(LOG_VERBOSE, "Socket %d: client connected.", slot);
        socket_state[slot].status |= SOCKET_CONNECTED;
        socket_state[slot].id = newid;
        //set to non-blocking again
        long nonblock = 1;
        int retval = sl_SetSockOpt(newid, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &nonblock, sizeof(nonblock));
        if(retval < 0){
            sl_Close(newid);
            RETURN_ERROR(retval);
        }
        if(slot == SOCKET_LOG){
            mem_log_start_putchar(&TS_LogSocketPutChar);
            serialconfig_start(&TS_LogSocketGetChar);
        }
    }
    else if(newid != SL_EAGAIN){
        sl_Close(socket_state[slot].id);
        sl_Close(newid);
        socket_state[slot].status = 0;
        RETURN_ERROR(newid);
    }

    return RET_SUCCESS;
}

int GetSockConnected(unsigned int slot){
    return (IS_SOCK_CONNECTED(slot) != 0);
}

void Task_SocketHandler(void* params)
{
    (void)params;
    //start socket server and accept a connection
    if(StartSockets() == RET_FAILURE) {TASK_RETURN_ERROR(ERROR_UNKNOWN);}

    for(;;){
        if(UpdateSockets() == RET_FAILURE) {TASK_RETURN_ERROR(ERROR_UNKNOWN);}
        vTaskDelay(1);
    }
}
