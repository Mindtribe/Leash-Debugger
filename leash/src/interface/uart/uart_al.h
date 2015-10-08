/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef UART_AL_H_
#define UART_AL_H_

void UartPutChar(char c);
void UartGetChar(char* c);
int UartCharsAvailable(void);
void UartInit(void);

#endif
