/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for NCO 01.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"

void SYSTEM_Initialize(void)
{
    INTCON0bits.GIE = 0;

    /* Port C: RC0 used as NCO demonstration output pin. */
    TRISCbits.TRISC0 = 0;
    ANSELCbits.ANSELC0 = 0;
    LATCbits.LATC0 = 0;

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
