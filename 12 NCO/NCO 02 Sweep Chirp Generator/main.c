/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for NCO 02 Sweep Chirp Generator.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdint.h>
#include "config.h"
#include "nco.h"

#define NCO_SWEEP_START     (0x000080UL)
#define NCO_SWEEP_STOP      (0x004000UL)
#define NCO_SWEEP_STEP      (0x000040UL)

void main(void)
{
    uint32_t increment = NCO_SWEEP_START;
    uint8_t direction_up = 1U;
    uint32_t settle;

    SYSTEM_Initialize();
    NCO1_Initialize();

    while (1)
    {
        NCO1_SetIncrement(increment);

        for (settle = 0U; settle < 25000UL; settle++)
        {
            NCO1_Task();
        }

        if (direction_up != 0U)
        {
            if ((increment + NCO_SWEEP_STEP) >= NCO_SWEEP_STOP)
            {
                increment = NCO_SWEEP_STOP;
                direction_up = 0U;
            }
            else
            {
                increment += NCO_SWEEP_STEP;
            }
        }
        else
        {
            if (increment <= (NCO_SWEEP_START + NCO_SWEEP_STEP))
            {
                increment = NCO_SWEEP_START;
                direction_up = 1U;
            }
            else
            {
                increment -= NCO_SWEEP_STEP;
            }
        }
    }
}
