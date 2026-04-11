/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for UART 01 Interrupt Echo Console.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "uart.h"

void main(void)
{
    int counter = 0; 
    SYSTEM_Initialize();
    UART1_Initialize();

    while (1)
    {
        printf("Loop %i\n", counter); 
        counter = counter + 1;
        __delay_ms(1000); 
    }
}
