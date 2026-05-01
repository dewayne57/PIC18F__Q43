/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for SMT 01.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"

void SYSTEM_Initialize(void)
{
    INTCON0bits.GIE = 0;

    /* Port B: RB0 input for measurement signal. */
    TRISBbits.TRISB0 = 1;
    ANSELBbits.ANSELB0 = 0;
    WPUBbits.WPUB0 = 0;

    /* Port D: LED status output for duty display. */
    TRISD = 0x00;
    ANSELD = 0x00;
    LATD = 0x00;

    /* Keep this project simple until UART diagnostics are added. */
    PMD0 = 0xFF;
    PMD1 = 0xFF;
    PMD2 = 0xFF;
    PMD3 = 0xFF;
    PMD4 = 0xFF;
    PMD5 = 0xFF;
    PMD6 = 0xFF;
    PMD7 = 0xFF;
    PMD8 = 0xFF;

    PMD0bits.SYSCMD = 0;
    PMD0bits.IOCMD = 0;

    INTCON0bits.GIE = 1;
}
