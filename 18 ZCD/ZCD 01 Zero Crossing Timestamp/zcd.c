/* *****************************************************************************************
 *   File Name: zcd.c
 *   Description: Module scaffold for ZCD 01 Zero Crossing Timestamp.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "zcd.h"

static uint16_t s_demo_counter;

void ZCD1_Initialize(void)
{
    s_demo_counter = 0U;

    /* TODO: Replace this software scaffold with real ZCD1 peripheral register setup. */
}

void ZCD1_Task(void)
{
    s_demo_counter++;

    if ((s_demo_counter & 0x03FFU) == 0U)
    {
        /* Heartbeat output for early bring-up visibility. */
        LATDbits.LATD0 ^= 1U;
    }
}
