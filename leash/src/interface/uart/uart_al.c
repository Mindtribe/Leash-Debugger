/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "uart_al.h"

//SDK includes
#include "uart.h"
#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "prcm.h"

//project includes
#include "error.h"

//FreeRTOS
#include "FreeRTOS.h"
#include "task.h"

#define UART_BAUD_RATE  115200
#define SYSCLK          80000000
#define CONSOLE         UARTA0_BASE
#define CONSOLE_PERIPH  PRCM_UARTA0

#define UART_BUF_SIZE 256
#define UART_INTERRUPT_FLAGS (UART_INT_RT | UART_INT_RX)

static char rxbuf[UART_BUF_SIZE];
static unsigned int rxbuf_iwrite = 0;
static unsigned int rxbuf_iread = 0;

static void UartIntHandler(void)
{

    long c;
    while((c = MAP_UARTCharGetNonBlocking(CONSOLE)) != -1){
        rxbuf[(rxbuf_iwrite++)%UART_BUF_SIZE] = (char) c;
        if(rxbuf_iwrite == rxbuf_iread) {
            ADD_ERROR(ERROR_OVERFLOW, "UART Overflow");
            rxbuf_iwrite--;
        }
    }
    MAP_UARTIntClear(CONSOLE, UART_INTERRUPT_FLAGS);
    return;
}

void UartInit(void)
{
    MAP_PRCMPeripheralClkEnable(CONSOLE_PERIPH, PRCM_RUN_MODE_CLK);
    MAP_UARTConfigSetExpClk(CONSOLE,MAP_PRCMPeripheralClockGet(CONSOLE_PERIPH),
            UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                    UART_CONFIG_PAR_NONE));

    //Register the UART interrupt
    MAP_UARTIntRegister(CONSOLE, &UartIntHandler);

    MAP_UARTIntEnable(CONSOLE, UART_INTERRUPT_FLAGS);

    return;
}

void UartPutChar(char c)
{
    MAP_UARTCharPut(CONSOLE,c);
    return;
}

void UartGetChar(char* c)
{
    while(rxbuf_iwrite == rxbuf_iread); //wait for a character to arrive
    vPortEnterCritical();
    *c = rxbuf[(rxbuf_iread++)%UART_BUF_SIZE];
    vPortExitCritical();
    return;
}

int UartCharsAvailable(void)
{
    return (rxbuf_iwrite != 0);
}
