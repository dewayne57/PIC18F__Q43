/* *****************************************************************************************
 *   File Name: timer.c
 *   Description: Module scaffold for TMR 01 Periodic Interrupt Tick.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "timer.h"

static uint16_t s_demo_counter;

void TMR0_Initialize(void)
{
    s_demo_counter = 0U;

    /* TODO: Replace this software scaffold with real TMR0 peripheral register setup. */
}

void TMR0_Task(void)
{
    s_demo_counter++;

    if ((s_demo_counter & 0x03FFU) == 0U)
    {
        /* Heartbeat output for early bring-up visibility. */
        LATDbits.LATD0 ^= 1U;
    }
}
