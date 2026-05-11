/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for NCO 01 Static Synthesizer.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdint.h>
#include "config.h"
#include "nco.h"

static const uint32_t k_profile_increments[] = {
    0x000100UL,
    0x000400UL,
    0x001000UL,
    0x004000UL
};

void main(void)
{
    uint8_t profile = 0U;
    uint32_t delay;

    SYSTEM_Initialize();
    NCO1_Initialize();

    while (1)
    {
        NCO1_SetIncrement(k_profile_increments[profile]);

        for (delay = 0U; delay < 30000UL; delay++)
        {
            NCO1_Task();
        }

        profile++;
        if (profile >= (uint8_t)(sizeof(k_profile_increments) / sizeof(k_profile_increments[0])))
        {
            profile = 0U;
        }
    }
}
