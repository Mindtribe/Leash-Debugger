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

#define NUM_SOCKETS 2

//status bits
#define SOCKET_STARTED 1
#define SOCKET_CONNECTED 2

#define IS_SOCK_STARTED(X) (socket_state[(X)].status & SOCKET_STARTED)
#define IS_SOCK_CONNECTED(X) (socket_state[(X)].status & SOCKET_CONNECTED)

struct socket_state_t{
    unsigned int status;
    int id;
    SlSockAddrIn_t addr_local;
    SlSockAddrIn_t addr_remote;
};

struct socket_state_t socket_state[NUM_SOCKETS] = {{0}};

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
