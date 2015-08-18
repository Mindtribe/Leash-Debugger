/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "gdbserver.h"
#include "target_al.h"
#include "gdb_helpers.h"
#include "common.h"
#include "error.h"
#include "mem_log.h"

enum gdbserver_packet_phase{
    PACKET_NONE = 0,
    PACKET_DATA,
    PACKET_CHECKSUM
};

struct gdbserver_state_t{
    int initialized;
    enum gdbserver_packet_phase packet_phase;
    int awaiting_ack;
    char last_sent_packet[MAX_GDB_PACKET_LEN];
    char cur_packet[MAX_GDB_PACKET_LEN];
    char cur_checksum[2];
    int cur_packet_index;
};
struct gdbserver_state_t gdbserver_state = {
    .initialized = 0,
    .packet_phase = PACKET_NONE,
    .awaiting_ack = 0,
    .last_sent_packet = {0},
    .cur_packet = {0},
    .cur_checksum = {0},
    .cur_packet_index = 0
};

int gdbserver_init(void)
{
    gdbserver_state.initialized = 1;
    return RET_SUCCESS;
}

int gdbserver_processChar(void)
{
    char c;
    gdb_helpers_GetChar(&c);

    mem_log_add("Got a char!", (int)c);
    switch(c){
    case '-':  //NACK (retransmit request)
    case '+': //ACK
        if(!gdbserver_state.awaiting_ack) { gdbserver_reset_error(__LINE__, ERROR_UNKNOWN); break; }
        else if(c == '+') { gdbserver_state.awaiting_ack = 0; } //confirm successful send
        else { gdb_helpers_TransmitPacket(gdbserver_state.last_sent_packet); }
        break;
    case '$':
        if(gdbserver_state.packet_phase != PACKET_NONE) { gdbserver_reset_error(__LINE__, ERROR_UNKNOWN); break; } //invalid character in packet
        gdbserver_state.packet_phase = PACKET_DATA;
        gdbserver_state.cur_packet_index = 0;
        break;
    case '#':
        if(gdbserver_state.packet_phase != PACKET_DATA) { gdbserver_reset_error(__LINE__, ERROR_UNKNOWN); break; } //invalid character at this time
        gdbserver_state.packet_phase = PACKET_CHECKSUM;
        gdbserver_state.cur_packet_index = 0;
        break;
    default:
        switch(gdbserver_state.packet_phase){
        case PACKET_NONE:
            gdbserver_reset_error(__LINE__, ERROR_UNKNOWN); break; //invalid character at this point.
            break;
        case PACKET_DATA:
            if(gdbserver_state.cur_packet_index >= (MAX_GDB_PACKET_LEN - 1)) {gdbserver_reset_error(__LINE__, ERROR_UNKNOWN); break;} //too long
            gdbserver_state.cur_packet[gdbserver_state.cur_packet_index++] = c;
            break;
        case PACKET_CHECKSUM:
            if(((c < '0') || (c > '9'))
                    && ((c < 'A') || (c > 'F'))) { gdbserver_reset_error(__LINE__, ERROR_UNKNOWN); break; } //checksum is hexadecimal chars only
            if(gdbserver_state.cur_packet_index == 0) {
                gdbserver_state.cur_checksum[0] = c;
                gdbserver_state.cur_packet_index++;
            }
            else{
                gdbserver_state.cur_checksum[1] = c;
                gdbserver_state.packet_phase = PACKET_NONE; //finished
                if(gdbserver_processPacket() == RET_FAILURE) gdbserver_reset_error(__LINE__, ERROR_UNKNOWN); //process the last received packet.
            }
            break;
        default:
            gdbserver_reset_error(__LINE__, ERROR_UNKNOWN); //should never reach this point
        }
        break;
    }

    return RET_SUCCESS;
}

void gdbserver_reset_error(int line, int error_code)
{
    error_add(__FILE__, line, error_code);
    gdbserver_state.packet_phase = PACKET_NONE;
    gdbserver_state.cur_packet_index = 0;
    gdbserver_state.awaiting_ack = 0;
}

int gdbserver_processPacket(void)
{
    //check the checksum
    if(gdb_helpers_hexToByte(gdbserver_state.cur_checksum) != gdb_helpers_getChecksum(gdbserver_state.cur_packet)){
        mem_log_add(gdbserver_state.cur_packet, 1);
        gdbserver_reset_error(__LINE__, ERROR_UNKNOWN);
        gdb_helpers_Nack();
    }

    //acknowledge
    gdb_helpers_Ack();

    //log the packet
    mem_log_add(gdbserver_state.cur_packet, 0);

    return RET_SUCCESS;
}

int gdbserver_loop(void)
{
    while(1){
        gdbserver_processChar();
    };

    return RET_SUCCESS;
}
