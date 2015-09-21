/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

//Serial-over-socket server functionality.

#ifndef SERIALSOCK_H_
#define SERIALSOCK_H_

#include "error.h"
#include "mem.h"
#include "log.h"

//These functions are for calling from the WiFi SimpleLink thread only.
int StartSerialSock(unsigned short port, unsigned int slot);
int SockAccept(unsigned int slot);
int GetSockConnected(unsigned int slot);
int StartSockets(void);
void StopSockets(void);
int UpdateSockets(void);
int SocketPutChar(char c, unsigned int socket_slot);
int SocketGetChar(char *c, unsigned int socket_slot);
int SocketRXCharAvailable(unsigned int socket_slot);
int SocketTXSpaceAvailable(unsigned int socket_slot);

//The 'TS_xxx' functions are meant for calling from other threads
//than the SimpleLink WiFi thread.
//Calling them from the WiFi thread itself may cause infinite stalls.
int TS_SocketPutChar(char c, unsigned int socket_slot);
int TS_SocketGetChar(char *c, unsigned int socket_slot);
int TS_SocketRXCharAvailable(unsigned int socket_slot);
int TS_SocketTXSpaceAvailable(unsigned int socket_slot);

#endif
