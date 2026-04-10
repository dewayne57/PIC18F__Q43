/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for TMR 01 Periodic Interrupt Tick.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "timer.h"

void main(void)
{
    SYSTEM_Initialize();
    TMR0_Initialize();

    while (1)
    {
        TMR0_Task();
    }
}
