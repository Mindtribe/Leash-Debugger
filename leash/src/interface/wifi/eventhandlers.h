/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

//Event handlers and callbacks for SimpleLink WiFi.

#ifndef EVENTHANDLERS_H_
#define EVENTHANDLERS_H_

#include "simplelink.h"

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent);
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent);
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse);
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent);
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock);

#endif
