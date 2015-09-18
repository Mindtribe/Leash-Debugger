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

int StartSerialSock(unsigned short port, unsigned int slot);

int SockAccept(unsigned int slot);

int GetSockConnected(unsigned int slot);

#endif
