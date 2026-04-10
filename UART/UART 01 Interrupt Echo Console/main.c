/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for UART 01 Interrupt Echo Console.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "uart.h"

void main(void)
{
    SYSTEM_Initialize();
    UART1_Initialize();

    while (1)
    {
        UART1_Task();
    }
}
