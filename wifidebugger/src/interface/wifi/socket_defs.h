/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef SOCKET_DEFS_H_
#define SOCKET_DEFS_H_

#define NUM_SOCKETS 3

enum{
    SOCKET_LOG = 0,
    SOCKET_TARGET,
    SOCKET_GDB
};

const int socket_ports[NUM_SOCKETS] = {
    49152,
    49153,
    49154
};

const char* socket_mdns_names_fixedpart[NUM_SOCKETS] = {
    "Debug Adapter Serial Link._raw-serial._tcp.local.",
    "Debug Target Serial Link._raw-serial._tcp.local.",
    "GDB Remote Link._gdbremote._tcp.local."
};

const char* socket_mdns_descriptions[NUM_SOCKETS] = {
    "Log information and interaction with WiFi debug adapter.",
    "Serial communication to WiFi debug adapter target.",
    "GDB connection to operate WiFi debug adapter."
};

#endif
