/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for NCO 02.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"

void SYSTEM_Initialize(void)
{
    INTCON0bits.GIE = 0;

    TRISCbits.TRISC0 = 0;
    ANSELCbits.ANSELC0 = 0;
    LATCbits.LATC0 = 0;

    INTCON0bits.GIE = 1;
}
