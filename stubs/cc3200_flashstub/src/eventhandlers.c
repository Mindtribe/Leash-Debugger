#include "eventhandlers.h"

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent)
{
    (void)pSlWlanEvent;
    return;
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    (void)pNetAppEvent;
    return;
}



void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
        SlHttpServerResponse_t *pHttpResponse)
{
    (void)pHttpEvent;
    (void)pHttpResponse;
    return;
}


void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    (void)pDevEvent;
    return;
}


void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    (void)pSock;
    return;
}
