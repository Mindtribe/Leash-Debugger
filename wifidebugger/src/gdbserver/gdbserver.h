/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

/*
 * Implementation of a server for GDB remote debugging protocol.
 */

#ifndef GDBSERVER_H_
#define GDBSERVER_H_

#include "target_al.h"

#define GDBSERVER_KEEP_CHARS //for debugging: whether to keep track of chars received
#define GDBSERVER_KEEP_CHARS_NUM 128 //for debugging: number of chars to keep track of

#define GDBSERVER_LOG_PACKETS

//note: keep <256 or change PacketSize response in gdbserver
#define GDBSERVER_MAX_PACKET_LEN_RX 256
#define GDBSERVER_MAX_PACKET_LEN_TX 256

#define GDBSERVER_MAX_BLOCK_ACCESS 256

#define GDBSERVER_NUM_BKPT 256

#define GDBSERVER_POLL_INTERVAL 100

#define CTRL_C 0x03 //interrupt character.

int gdbserver_init(void (*pPutChar)(char), void (*pGetChar)(char*), int (*pGetCharsAvail)(void), struct target_al_interface *target);

int gdbserver_processChar(void);

int gdbserver_processPacket(void);

void gdbserver_reset_error(int line, int error_code);

int gdbserver_loop(void);

void gdbserver_TransmitPacket(char* packet_data);

void gdbserver_TransmitStopReason(void);

int gdbserver_readMemory(char* argstring);

int gdbserver_writeMemory(char* argstring, uint8_t binary_format);

void gdbserver_Interrupt(uint8_t signal);

int gdbserver_doMemCRC(char* argstring);

int gdbserver_setSWBreakpoint(uint32_t addr, uint8_t len_bytes);

int gdbserver_pollTarget(void);

int gdbserver_handleHalt(void);

int gdbserver_continue(void);

int gdbserver_handleSemiHosting(void);

#endif
