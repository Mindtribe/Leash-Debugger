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

#include <stdint.h>

#define GDBSERVER_TASK_STACK_SIZE 2048
#define GDBSERVER_TASK_PRIORITY 5

struct target_al_interface;

int gdbserver_init(void (*pPutChar)(char), void (*pGetChar)(char*), int (*pGetCharsAvail)(void), struct target_al_interface *target);

int gdbserver_processChar(void);

int gdbserver_processPacket(void);

void gdbserver_reset_error(int line, int error_code);

void gdbserver_loop_task(void* params);

void gdbserver_TransmitPacket(char* packet_data);

void gdbserver_TransmitBinaryPacket(unsigned char* packet_data, unsigned int len);

void gdbserver_TransmitStopReason(void);

int gdbserver_readMemory(char* argstring);

int gdbserver_writeMemory(char* argstring);

int gdbserver_writeMemoryBinary(char* argstring);

void gdbserver_Interrupt(uint8_t signal);

int gdbserver_doMemCRC(char* argstring);

int gdbserver_setSWBreakpoint(uint32_t addr, uint8_t len_bytes);

int gdbserver_pollTarget(void);

int gdbserver_handleHalt(void);

int gdbserver_continue(void);

int gdbserver_handleSemiHosting(void);

void gdbserver_sendInfo(void);

int gdbserver_TransmitFileIOWrite(uint8_t descriptor, char *buf, uint32_t count);

#endif
