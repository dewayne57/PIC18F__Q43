/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for UART 01 Interrupt Echo Console.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 * 
 *   Configure Port B pins for use by UART 1 as follows: 
 *   RB0 - TX1 (output)
 *   RB1 - RX1 (input)
 *   RB2 - CTS1 (input)
 *   RB3 - RTS1 (output)
 * 
 *  The demonstration will use the internal 64MHz high frequency oscillator as the system clock.
 *  The PIC 18F47Q43 has a built in baud rate generator that can be used to generate the required
 *  baud rates for the UART module. The baud rate generator can be configured to use the system clock
 *  or an external clock source. In this demonstration, we will use the system clock as the source for the baud rate generator.
 *  The baud rate generator can be configured to generate baud rates up to 115200 baud with a system clock of 64MHz.
 *  The UART module will be configured to use the baud rate generator to generate the required baud
 *  rates for the demonstration.
 *  The UART module will be configured to use the internal clock source for the baud rate generator,
 *  and the baud rate will be set to 9600 baud, No parity, 8 data bits, and 1 stop bit. 
 * 
 *   Note, this will require the use of the PPS system to assign the UART 1 functions to 
 *   the appropriate pins.
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "uart.h"

void SYSTEM_Initialize(void)
{
    INTCON0bits.GIE = 0;

    /* Keep all demonstration pins digital. */
    ANSELB = 0x00;
    ANSELC = 0x00;
    ANSELD = 0x00;

    INTCON0bits.GIE = 1;
}

