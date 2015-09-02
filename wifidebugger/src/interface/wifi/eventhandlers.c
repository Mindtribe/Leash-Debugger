#include "eventhandlers.h"
#include "simplelink_defs.h"
#include "wifi.h"
#include "mem_log.h"

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent)
{
    switch(pSlWlanEvent->Event)
    {
    case SL_WLAN_CONNECT_EVENT:
    {
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
            mem_log_add("Application disconnected", 0);
        }
        else{
            mem_log_add("Unexpected disconnect", 0);
        }
    }
    break;

    case SL_WLAN_STA_CONNECTED_EVENT:
    {
        // when device is in AP mode and any client connects to device cc3xxx
        SET_STATUS_BIT(wifi_state.status, STATUS_BIT_CONNECTION);
    }
    break;

    case SL_WLAN_STA_DISCONNECTED_EVENT:
    {
        // when client disconnects from device (AP)
        CLR_STATUS_BIT(wifi_state.status, STATUS_BIT_CONNECTION);
        CLR_STATUS_BIT(wifi_state.status, STATUS_BIT_IP_LEASED);
    }
    break;

    default:
    {
        mem_log_add("[WLAN EVENT] Unexpected event", 0);
    }
    break;
    }
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    switch(pNetAppEvent->Event)
    {
    case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
    case SL_NETAPP_IPV6_IPACQUIRED_EVENT:
    {
        SET_STATUS_BIT(wifi_state.status, STATUS_BIT_IP_AQUIRED);
    }
    break;

    case SL_NETAPP_IP_LEASED_EVENT:
    {
        SET_STATUS_BIT(wifi_state.status, STATUS_BIT_IP_LEASED);

        wifi_state.station_IP = (pNetAppEvent)->EventData.ipLeased.ip_address;

        mem_log_add("[NETAPP_EVENT] IP leased to Client.", 0);

        /*TODO: improve
        UART_PRINT("[NETAPP EVENT] IP Leased to Client: IP=%d.%d.%d.%d , ",
                SL_IPV4_BYTE(wifi_state.station_IP,3), SL_IPV4_BYTE(wifi_state.station_IP,2),
                SL_IPV4_BYTE(wifi_state.station_IP,1), SL_IPV4_BYTE(wifi_state.station_IP,0));*/
    }
    break;

    case SL_NETAPP_IP_RELEASED_EVENT:
    {
        CLR_STATUS_BIT(wifi_state.status, STATUS_BIT_IP_LEASED);

        mem_log_add("[NETAPP_EVENT] IP Released for Client.", 0);

        /*TODO: improve
        UART_PRINT("[NETAPP EVENT] IP Released for Client: IP=%d.%d.%d.%d , ",
                SL_IPV4_BYTE(wifi_state.station_IP,3), SL_IPV4_BYTE(wifi_state.station_IP,2),
                SL_IPV4_BYTE(wifi_state.station_IP,1), SL_IPV4_BYTE(wifi_state.station_IP,0));*/

    }
    break;

    default:
    {

        mem_log_add("[NETAPP_EVENT] Unexpected event.", 0);

        /*TODO: improve
        UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                pNetAppEvent->Event);*/
    }
    break;
    }
}



void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
        SlHttpServerResponse_t *pHttpResponse)
{
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
    mem_log_add("[GENERAL_EVENT] Unknown.", 0);

    /*TODO: improve
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
            pDevEvent->EventData.deviceEvent.status,
            pDevEvent->EventData.deviceEvent.sender);*/
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
            mem_log_add("[SOCK_ERROR] Failed to transmit all queued packets before closing.", 0);
            /*UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                    "failed to transmit all queued packets\n\n",
                    pSock->socketAsyncEvent.SockTxFailData.sd);*/
            break;
        default:
            mem_log_add("[SOCK_ERROR] Failed to transmit.", 0);
            /*UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , reason "
                    "(%d) \n\n",
                    pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);*/
            break;
        }
        break;

        default:
            mem_log_add("[SOCK_EVENT] Unexpected event.", 0);
            /*
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);*/
            break;
    }

}
