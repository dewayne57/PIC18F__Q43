/* *****************************************************************************************
 *   File Name: i2c.c
 *   Description: Module scaffold for I2C 01 Host Transaction Basics.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "i2c.h"

static uint16_t s_demo_counter;

void I2C1_Initialize(void)
{
    s_demo_counter = 0U;

    /* TODO: Replace this software scaffold with real I2C1 peripheral register setup. */
}

void I2C1_Task(void)
{
    s_demo_counter++;

    if ((s_demo_counter & 0x03FFU) == 0U)
    {
        /* Heartbeat output for early bring-up visibility. */
        LATDbits.LATD0 ^= 1U;
    }
}
