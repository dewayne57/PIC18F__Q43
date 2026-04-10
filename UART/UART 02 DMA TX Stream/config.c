/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for UART 02 DMA TX Stream.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"

void SYSTEM_Initialize(void)
{
    INTCON0bits.GIE = 0;

    /* Keep all candidate IO pins digital for bring-up. */
    ANSELB = 0x00;
    ANSELC = 0x00;
    ANSELD = 0x00;

    /* UART1 TX pin candidate (RC6) as digital output. */
    TRISCbits.TRISC6 = 0;

    TRISD = 0x00;
    LATD = 0x00;

    INTCON0bits.GIE = 1;
}
