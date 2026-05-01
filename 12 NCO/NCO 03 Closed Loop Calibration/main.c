/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for NCO 03 Closed Loop Calibration.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdint.h>
#include "config.h"
#include "nco.h"

#define NCO_TARGET_INCREMENT    (0x002000UL)
#define NCO_CAL_STEP            (0x000008UL)

static uint32_t APP_ReadMeasuredIncrementEstimate(void)
{
    /* Placeholder: replace with SMT-based measured estimate when SMT project is integrated. */
    return NCO1_GetIncrement();
}

void main(void)
{
    uint32_t measured;
    uint32_t current;
    uint32_t settle;

    SYSTEM_Initialize();
    NCO1_Initialize();

    while (1)
    {
        measured = APP_ReadMeasuredIncrementEstimate();
        current = NCO1_GetIncrement();

        if (measured < NCO_TARGET_INCREMENT)
        {
            NCO1_SetIncrement(current + NCO_CAL_STEP);
        }
        else if (measured > NCO_TARGET_INCREMENT)
        {
            NCO1_SetIncrement(current - NCO_CAL_STEP);
        }

        for (settle = 0U; settle < 15000UL; settle++)
        {
            NCO1_Task();
        }
    }
}
