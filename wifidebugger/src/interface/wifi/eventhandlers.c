#include "eventhandlers.h"

#include "log.h"
#include "simplelink_defs.h"
#include "wifi.h"

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent)
{
    switch(pSlWlanEvent->Event)
    {
    case SL_WLAN_CONNECT_EVENT:
    {
        LOG(LOG_IMPORTANT, "[WIFI] Application connected");
        SET_STATUS_BIT(wifi_state.status, STATUS_BIT_CONNECTION);
    }
    break;

    case SL_WLAN_DISCONNECT_EVENT:
    {
        slWlanConnectAsyncResponse_t*  pEventData = NULL;

        CLR_STATUS_BIT(wifi_state.status, STATUS_BIT_CONNECTION);
        CLR_STATUS_BIT(wifi_state.status, STATUS_BIT_IP_AQUIRED);

        pEventData = &pSlWlanEvent->EventData.STAandP2PModeDisconnected;

        // If the user has initiated 'Disconnect' request,
        //'reason_code' is SL_USER_INITIATED_DISCONNECTION
        if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code){
            LOG(LOG_IMPORTANT, "[WIFI] Application disconnected");
        }
        else{
            LOG(LOG_IMPORTANT, "[WIFI] Unexpected disconnect");
        }
    }
    break;

    case SL_WLAN_STA_CONNECTED_EVENT:
    {
        LOG(LOG_IMPORTANT, "[WIFI] Station connected to CC3200 AP");
        // when device is in AP mode and any client connects to device cc3xxx
        SET_STATUS_BIT(wifi_state.status, STATUS_BIT_CONNECTION);
    }
    break;

    case SL_WLAN_STA_DISCONNECTED_EVENT:
    {
        LOG(LOG_IMPORTANT, "[WIFI] Station disconnected from CC3200 AP");
        // when client disconnects from device (AP)
        CLR_STATUS_BIT(wifi_state.status, STATUS_BIT_CONNECTION);
        CLR_STATUS_BIT(wifi_state.status, STATUS_BIT_IP_LEASED);
    }
    break;

    default:
    {
        LOG(LOG_IMPORTANT, "[WIFI] Unexpected WLAN event: 0x%8X", (unsigned int) pSlWlanEvent->Event);
    }
    break;
    }
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    switch(pNetAppEvent->Event)
    {
    case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
    {
        wifi_state.self_IP = (pNetAppEvent)->EventData.ipAcquiredV4.ip;
        LOG(LOG_VERBOSE, "[WIFI] NetApp: IPv4 Acquired: %d.%d.%d.%d",
                (unsigned int)SL_IPV4_BYTE(wifi_state.self_IP,3), (unsigned int) SL_IPV4_BYTE(wifi_state.self_IP,2),
                (unsigned int)SL_IPV4_BYTE(wifi_state.self_IP,1), (unsigned int) SL_IPV4_BYTE(wifi_state.self_IP,0));
        SET_STATUS_BIT(wifi_state.status, STATUS_BIT_IP_AQUIRED);
    }
    break;

    case SL_NETAPP_IPV6_IPACQUIRED_EVENT:
    {
        LOG(LOG_VERBOSE, "[WIFI] NetApp: IPv6 Acquired");
        SET_STATUS_BIT(wifi_state.status, STATUS_BIT_IP_AQUIRED);
    }
    break;

    case SL_NETAPP_IP_LEASED_EVENT:
    {
        SET_STATUS_BIT(wifi_state.status, STATUS_BIT_IP_LEASED);

        wifi_state.client_IP = (pNetAppEvent)->EventData.ipLeased.ip_address;

        LOG(LOG_IMPORTANT, "[WIFI] NetApp: IP leased to Client: %d.%d.%d.%d",
                (unsigned int)SL_IPV4_BYTE(wifi_state.client_IP,3), (unsigned int) SL_IPV4_BYTE(wifi_state.client_IP,2),
                (unsigned int)SL_IPV4_BYTE(wifi_state.client_IP,1), (unsigned int) SL_IPV4_BYTE(wifi_state.client_IP,0));
    }
    break;

    case SL_NETAPP_IP_RELEASED_EVENT:
    {
        CLR_STATUS_BIT(wifi_state.status, STATUS_BIT_IP_LEASED);

        LOG(LOG_IMPORTANT, "[WIFI] NetApp: IP Released for Client: %d.%d.%d.%d",
                (unsigned int)SL_IPV4_BYTE(wifi_state.client_IP,3), (unsigned int) SL_IPV4_BYTE(wifi_state.client_IP,2),
                (unsigned int)SL_IPV4_BYTE(wifi_state.client_IP,1), (unsigned int) SL_IPV4_BYTE(wifi_state.client_IP,0));

        wifi_state.client_IP = 0;
    }
    break;

    default:
    {

        LOG(LOG_IMPORTANT, "[WIFI] Unexpected event: 0x%8X", (unsigned int)pNetAppEvent->Event);
    }
    break;
    }
}



void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
        SlHttpServerResponse_t *pHttpResponse)
{
    LOG(LOG_VERBOSE, "[WIFI] HTTP Server Callback Triggered");
    (void)pHttpEvent;
    (void)pHttpResponse;
    // Unused in this application
}


void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    (void)pDevEvent;

    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //

    LOG(LOG_VERBOSE, "[WIFI] Unknown General Event - ID=%d Sender=%d",
            pDevEvent->EventData.deviceEvent.status,
            pDevEvent->EventData.deviceEvent.sender);
}


void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    //
    // This application doesn't work w/ socket - Events are not expected
    //
    switch( pSock->Event )
    {
    case SL_SOCKET_TX_FAILED_EVENT:
        switch( pSock->socketAsyncEvent.SockTxFailData.status)
        {
        case SL_ECLOSE:
            LOG(LOG_IMPORTANT,
                    "[WIFI] SockErr: close socket (%d) operation failed to transmit all queued packets",
                    pSock->socketAsyncEvent.SockTxFailData.sd);
            break;
        default:
            LOG(LOG_IMPORTANT,
                    "[WIFI] SockErr: - TX FAILED  :  socket %d , reason (%d)",
                    pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);
            break;
        }
        break;

        default:
            LOG(LOG_IMPORTANT, "[WIFI] SockEvent: Unexpected event [0x%8X]\n\n", (unsigned int) pSock->Event);
            break;
    }

}
