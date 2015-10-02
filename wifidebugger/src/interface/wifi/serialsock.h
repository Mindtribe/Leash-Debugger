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

#define SOCKET_TASK_PRIORITY (3)
#define SOCKET_TASK_STACK_SIZE (2048)

#define NUM_SOCKETS 3
extern const int socket_ports[NUM_SOCKETS];
extern const char* socket_mdns_names_fixedpart[NUM_SOCKETS];
extern const char* socket_mdns_descriptions[NUM_SOCKETS];

//These functions are for calling from the WiFi SimpleLink thread only.
int StartSerialSock(unsigned short port, unsigned int slot);
int SockAccept(unsigned int slot);
int GetSockConnected(unsigned int slot);
int StartSockets(void);
void InitSockets(void);
void StopSockets(void);
int UpdateSockets(void);
int SocketPutChar(char c, unsigned int socket_slot);
int SocketGetChar(char *c, unsigned int socket_slot);
int SocketRXCharAvailable(unsigned int socket_slot);
int SocketTXSpaceAvailable(unsigned int socket_slot);

//The 'TS_xxx' functions are meant for calling from other threads
//than the SimpleLink WiFi thread.
//Calling them from the WiFi thread itself may cause infinite stalls.
void TS_LogSocketPutChar(char c);
void TS_TargetSocketPutChar(char c);
void TS_GDBSocketPutChar(char c);
void TS_LogSocketGetChar(char *c);
void TS_TargetSocketGetChar(char *c);
void TS_GDBSocketGetChar(char *c);
int TS_SocketPutChar(char c, unsigned int socket_slot);
int TS_SocketGetChar(char *c, unsigned int socket_slot);
int TS_SocketRXCharAvailable(unsigned int socket_slot);
int TS_SocketTXSpaceAvailable(unsigned int socket_slot);
int TS_LogSocketRXCharAvailable(void);
int TS_GDBSocketRXCharAvailable(void);
int TS_TargetSocketRXCharAvailable(void);
int TS_LogSocketTXSpaceAvailable(void);
int TS_GDBSocketTXSpaceAvailable(void);
int TS_TargetSocketTXSpaceAvailable(void);

//Task to handle all socket-related operations.
void Task_SocketHandler(void* params);

#endif
