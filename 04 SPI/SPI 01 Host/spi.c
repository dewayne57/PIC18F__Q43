/* *****************************************************************************************
 *   File Name: spi.c
 *   Description: Module scaffold for SPI 01 Host Loopback Transfer.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "spi.h"

static uint16_t s_demo_counter;

void SPI1_Initialize(void)
{
    s_demo_counter = 0U;

    /* TODO: Replace this software scaffold with real SPI1 peripheral register setup. */
}

void SPI1_Task(void)
{
    s_demo_counter++;

    if ((s_demo_counter & 0x03FFU) == 0U)
    {
        /* Heartbeat output for early bring-up visibility. */
        LATDbits.LATD0 ^= 1U;
    }
}
