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

//The 'ExtThread_xxx' functions are meant for calling from other threads
//than the SimpleLink socket server thread.
//Calling them from the socket server thread itself may cause lock-up.

//Insert a char into the send buffer of the corresponding socket.
//Blocks until space becomes available.
void ExtThread_LogSocketPutChar(char c);
void ExtThread_TargetSocketPutChar(char c);
void ExtThread_GDBSocketPutChar(char c);

//Get a char from the receive buffer of the corresponding socket.
//Blocks until a character becomes available.
void ExtThread_LogSocketGetChar(char *c);
void ExtThread_TargetSocketGetChar(char *c);
void ExtThread_GDBSocketGetChar(char *c);

//Check how many characters are available in the corresponding
//socket's receive buffer.
int ExtThread_LogSocketRXCharAvailable(void);
int ExtThread_GDBSocketRXCharAvailable(void);
int ExtThread_TargetSocketRXCharAvailable(void);

//Check how many free spots are available in the corresponding
//socket's send buffer.
int ExtThread_LogSocketTXSpaceAvailable(void);
int ExtThread_GDBSocketTXSpaceAvailable(void);
int ExtThread_TargetSocketTXSpaceAvailable(void);

//Generic implementations of the putting and getting functions.
//Functionally equal, except the socket is selectable.
int ExtThread_SocketPutChar(char c, unsigned int socket_slot);
int ExtThread_SocketGetChar(char *c, unsigned int socket_slot);

//Generic implementation of the space/character checking functions.
//Functionally equal, except the socket is selectable.
int ExtThread_SocketRXCharAvailable(unsigned int socket_slot);
int ExtThread_SocketTXSpaceAvailable(unsigned int socket_slot);

//Get the number of sockets maintained by the server.
int ExtThread_GetNumSockets(void);

//Get the port of a socket.
int ExtThread_GetSocketPort(int socket);

//Get strings depicting mDNS attributes associated with a socket.
const char* ExtThread_GetSocketMDNSName(int socket);
const char* ExtThread_GetSocketMDNSDesc(int socket);

//FreeRTOS task to handle all socket-related operations.
void Task_SocketServer(void* params);

//Initialize sockets: call before any thread tries to
//access the socket server through ExtThread_ functions
//and before Task_SocketServer is started.
void InitSockets(void);

#endif
