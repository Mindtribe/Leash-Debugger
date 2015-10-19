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

//THE FUNCTIONS BELOW SHOULD ONLY BE CALLED FROM THE SOCKET SERVER'S OWN THREAD
void InitSockets(void); //Initialize the socket server.
int StartSockets(void); //Start the socket server.
void StopSockets(void); //Stop the socket server.
int UpdateSockets(void); //Update the socket server in a polling fashion
                            //(must be called repeatedly for the server to work).


//The 'TS_xxx' functions are meant for calling from other threads
//than the SimpleLink WiFi thread.
//Calling them from the WiFi thread itself may cause infinite stalls.

//Insert a char into the send buffer of the corresponding socket.
//Blocks until space becomes available.
void TS_LogSocketPutChar(char c);
void TS_TargetSocketPutChar(char c);
void TS_GDBSocketPutChar(char c);

//Get a char from the receive buffer of the corresponding socket.
//Blocks until a character becomes available.
void TS_LogSocketGetChar(char *c);
void TS_TargetSocketGetChar(char *c);
void TS_GDBSocketGetChar(char *c);

//Check how many characters are available in the corresponding
//socket's receive buffer.
int TS_LogSocketRXCharAvailable(void);
int TS_GDBSocketRXCharAvailable(void);
int TS_TargetSocketRXCharAvailable(void);

//Check how many free spots are available in the corresponding
//socket's send buffer.
int TS_LogSocketTXSpaceAvailable(void);
int TS_GDBSocketTXSpaceAvailable(void);
int TS_TargetSocketTXSpaceAvailable(void);

//Generic implementations of the putting and getting functions.
//Functionally equal, except the socket is selectable.
int TS_SocketPutChar(char c, unsigned int socket_slot);
int TS_SocketGetChar(char *c, unsigned int socket_slot);

//Generic implementation of the space/character checking functions.
//Functionally equal, except the socket is selectable.
int TS_SocketRXCharAvailable(unsigned int socket_slot);
int TS_SocketTXSpaceAvailable(unsigned int socket_slot);

//Get the number of sockets maintained by the server.
int TS_GetNumSockets(void);

//Get the port of a socket.
int TS_GetSocketPort(int socket);

//Get strings depicting mDNS attributes associated with a socket.
const char* TS_GetSocketMDNSName(int socket);
const char* TS_GetSocketMDNSDesc(int socket);

//FreeRTOS task to handle all socket-related operations.
void Task_SocketHandler(void* params);

#endif
