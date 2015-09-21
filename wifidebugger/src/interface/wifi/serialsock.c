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
#include "FreeRTOS.h"
#include "semphr.h"

#define NUM_SOCKETS 3

//status bits
#define SOCKET_STARTED 1
#define SOCKET_CONNECTED 2

#define IS_SOCK_STARTED(X) (socket_state[(X)].status & SOCKET_STARTED)
#define IS_SOCK_CONNECTED(X) (socket_state[(X)].status & SOCKET_CONNECTED)

#define SOCKET_TXBUF_SIZE 128
#define SOCKET_RXBUF_SIZE 128

//Note: be aware that arguments to this macro get evaluated twice.
//(so no MIN(x++, y++)!)
#define MIN(X, Y) ((X) < (Y)) ? (X) : (Y)

enum{
    SOCKET_LOG = 0,
    SOCKET_TARGET,
    SOCKET_GDB
};

const int socket_ports[NUM_SOCKETS] = {
    1000,
    1001,
    1002
};

struct socket_state_t{
    unsigned int status;
    int id;
    SlSockAddrIn_t addr_local;
    SlSockAddrIn_t addr_remote;
    SemaphoreHandle_t bufmutex;
    char tx_buf[SOCKET_TXBUF_SIZE];
    unsigned int tx_buf_i_in;
    unsigned int tx_buf_i_out;
    unsigned int tx_buf_occupancy;
    char rx_buf[SOCKET_RXBUF_SIZE];
    unsigned int rx_buf_i_in;
    unsigned int rx_buf_i_out;
    unsigned int rx_buf_occupancy;
};

struct socket_state_t socket_state[NUM_SOCKETS] = {{0}};

int StartSockets(void)
{
    for(int i=0; i<NUM_SOCKETS; i++){
        if(StartSerialSock(socket_ports[i], i) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        if(socket_state[i].bufmutex == NULL) socket_state[i].bufmutex = xSemaphoreCreateMutex();
    }
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
        xSemaphoreTake(socket_state[socket_slot].bufmutex, portMAX_DELAY);
        if(socket_state[socket_slot].tx_buf_occupancy < SOCKET_TXBUF_SIZE) {break;}
        xSemaphoreGive(socket_state[socket_slot].bufmutex);
        UpdateSockets();
    }

    socket_state[socket_slot].tx_buf[(socket_state[socket_slot].tx_buf_i_in++)%SOCKET_TXBUF_SIZE] = c;
    socket_state[socket_slot].tx_buf_occupancy++;
    xSemaphoreGive(socket_state[socket_slot].bufmutex);

    return RET_SUCCESS;
}

int SocketGetChar(char *c, unsigned int socket_slot)
{
    for(;;){
        xSemaphoreTake(socket_state[socket_slot].bufmutex, portMAX_DELAY);
        if(socket_state[socket_slot].rx_buf_occupancy > 0) {break;}
        xSemaphoreGive(socket_state[socket_slot].bufmutex);
        UpdateSockets();
    }

    *c = socket_state[socket_slot].rx_buf[(socket_state[socket_slot].rx_buf_i_out++)%SOCKET_RXBUF_SIZE];
    socket_state[socket_slot].rx_buf_occupancy--;
    xSemaphoreGive(socket_state[socket_slot].bufmutex);

    return RET_SUCCESS;
}

int TS_SocketPutChar(char c, unsigned int socket_slot)
{
    for(;;){
        xSemaphoreTake(socket_state[socket_slot].bufmutex, portMAX_DELAY);
        if(socket_state[socket_slot].tx_buf_occupancy < SOCKET_TXBUF_SIZE) {break;}
        xSemaphoreGive(socket_state[socket_slot].bufmutex);
    }

    socket_state[socket_slot].tx_buf[(socket_state[socket_slot].tx_buf_i_in++)%SOCKET_TXBUF_SIZE] = c;
    socket_state[socket_slot].tx_buf_occupancy++;
    xSemaphoreGive(socket_state[socket_slot].bufmutex);

    return RET_SUCCESS;
}

int TS_SocketGetChar(char *c, unsigned int socket_slot)
{
    for(;;){
        xSemaphoreTake(socket_state[socket_slot].bufmutex, portMAX_DELAY);
        if(socket_state[socket_slot].rx_buf_occupancy > 0) {break;}
        xSemaphoreGive(socket_state[socket_slot].bufmutex);
    }

    *c = socket_state[socket_slot].rx_buf[(socket_state[socket_slot].rx_buf_i_out++)%SOCKET_RXBUF_SIZE];
    socket_state[socket_slot].rx_buf_occupancy--;
    xSemaphoreGive(socket_state[socket_slot].bufmutex);

    return RET_SUCCESS;
}

int SocketRXCharAvailable(unsigned int socket_slot){
    int occupancy;
    xSemaphoreTake(socket_state[socket_slot].bufmutex, portMAX_DELAY);
    occupancy = socket_state[socket_slot].rx_buf_occupancy;
    xSemaphoreGive(socket_state[socket_slot].bufmutex);

    return occupancy;
}

int SocketTXSpaceAvailable(unsigned int socket_slot){
    int space;
    xSemaphoreTake(socket_state[socket_slot].bufmutex, portMAX_DELAY);
    space = SOCKET_TXBUF_SIZE - socket_state[socket_slot].tx_buf_occupancy;
    xSemaphoreGive(socket_state[socket_slot].bufmutex);

    return space;
}

int TS_SocketRXCharAvailable(unsigned int socket_slot){
    int occupancy;
    xSemaphoreTake(socket_state[socket_slot].bufmutex, portMAX_DELAY);
    occupancy = socket_state[socket_slot].rx_buf_occupancy;
    xSemaphoreGive(socket_state[socket_slot].bufmutex);

    return occupancy;
}

int TS_SocketTXSpaceAvailable(unsigned int socket_slot){
    int space;
    xSemaphoreTake(socket_state[socket_slot].bufmutex, portMAX_DELAY);
    space = SOCKET_TXBUF_SIZE - socket_state[socket_slot].tx_buf_occupancy;
    xSemaphoreGive(socket_state[socket_slot].bufmutex);

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
            xSemaphoreTake(socket_state[i].bufmutex, portMAX_DELAY);

            //Update TX
            if(socket_state[i].tx_buf_occupancy){
                retval = sl_Send(socket_state[i].id,
                        &(socket_state[i].tx_buf[socket_state[i].tx_buf_i_out % SOCKET_RXBUF_SIZE]),
                        MIN(socket_state[i].tx_buf_occupancy,
                                SOCKET_TXBUF_SIZE - (socket_state[i].tx_buf_i_out % SOCKET_RXBUF_SIZE)), //Prevent overflow at wraparound point
                                0);
                if(retval<0) {xSemaphoreGive(socket_state[i].bufmutex); RETURN_ERROR(retval);}
                else{
                    socket_state[i].tx_buf_occupancy -= retval;
                    socket_state[i].tx_buf_i_out += retval;
                }
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
            }
            else if((retval<0) && (retval!=SL_EAGAIN)) {xSemaphoreGive(socket_state[i].bufmutex); RETURN_ERROR(retval);}


            xSemaphoreGive(socket_state[i].bufmutex);
        }
    }

    return RET_SUCCESS;
}

int StartSerialSock(unsigned short port, unsigned int slot)
{
    if(!IS_CONNECTED(wifi_state.status) || !IS_IP_ACQUIRED(wifi_state.status)) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(IS_SOCK_STARTED(slot)) {RETURN_ERROR(ERROR_UNKNOWN);}

    LOG(LOG_VERBOSE, "Starting socket %d on port %d...", slot, port);

    int retval;
    long nonblock = 1;

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

    retval = sl_Listen(socket_state[slot].id, 0);
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
