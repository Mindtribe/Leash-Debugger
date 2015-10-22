/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef UART_AL_H_
#define UART_AL_H_

//Put a single character in the UART send buffer,
//to be sent automatically. Blocks until space
//frees up.
void UartPutChar(char c);

//Get a single character from the UART receive buffer.
//Blocks until a character becomes available.
void UartGetChar(char* c);

//Peek into the receive buffer. Returns the number of
//unread characters in the receive buffer.
int UartCharsAvailable(void);

//Initialize the UART interface.
void UartInit(void);

#endif
