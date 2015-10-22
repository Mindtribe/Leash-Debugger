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

//Called by the SimpleLink driver to notify of WLAN events.
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent);

//Called by the SimpleLink driver to notify of NetApp events.
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent);

//Called by the SimpleLink driver to notify of HTTP server events.
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse);

//Called by the SimpleLink driver to notify of general events.
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent);

//Called by the SimpleLink driver to notify of socket system events.
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock);

#endif
