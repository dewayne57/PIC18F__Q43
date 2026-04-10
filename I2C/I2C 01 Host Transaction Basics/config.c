/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for I2C 01 Host Transaction Basics.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"

void SYSTEM_Initialize(void)
{
    INTCON0bits.GIE = 0;

    /* Keep all demonstration pins digital. */
    ANSELB = 0x00;
    ANSELC = 0x00;
    ANSELD = 0x00;

    /* Default outputs off for indicator port. */
    TRISD = 0x00;
    LATD = 0x00;

    INTCON0bits.GIE = 1;
}
