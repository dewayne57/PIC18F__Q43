/* *****************************************************************************************
 *   File Name: cmp.c
 *   Description: Module scaffold for CMP 01 Threshold Interrupt Event.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "cmp.h"

static uint16_t s_demo_counter;

void CMP1_Initialize(void)
{
    s_demo_counter = 0U;

    /* TODO: Replace this software scaffold with real CMP1 peripheral register setup. */
}

void CMP1_Task(void)
{
    s_demo_counter++;

    if ((s_demo_counter & 0x03FFU) == 0U)
    {
        /* Heartbeat output for early bring-up visibility. */
        LATDbits.LATD0 ^= 1U;
    }
}
