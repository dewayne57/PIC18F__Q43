/* *****************************************************************************************
 *   File Name: nco.c
 *   Description: Hardware NCO1 setup and increment control.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdint.h>
#include "nco.h"

static uint32_t s_increment;

static uint32_t NCO1_ClampIncrement(uint32_t increment)
{
    return (increment & 0x000FFFFFUL);
}

static void NCO1_WriteIncrement(uint32_t increment)
{
    uint32_t clamped = NCO1_ClampIncrement(increment);

#ifdef NCO1INCL
    NCO1INCL = (uint8_t)(clamped & 0xFFU);
#endif
#ifdef NCO1INCH
    NCO1INCH = (uint8_t)((clamped >> 8) & 0xFFU);
#endif
#ifdef NCO1INCU
    NCO1INCU = (uint8_t)((clamped >> 16) & 0x1FU);
#endif
}

void NCO1_Initialize(void)
{
    s_increment = 0x001000UL;

#ifdef RC0PPS
    RC0PPS = 0x3FU;
#endif
#ifdef NCO1CON
    NCO1CON = 0x00U;
#endif
#ifdef NCO1CLK
    NCO1CLK = 0x01U;
#endif

    NCO1_WriteIncrement(s_increment);

#ifdef NCO1CONbits
    NCO1CONbits.EN = 1;
#endif
}

void NCO1_SetIncrement(uint32_t increment)
{
    s_increment = NCO1_ClampIncrement(increment);
    NCO1_WriteIncrement(s_increment);
}

uint32_t NCO1_GetIncrement(void)
{
    return s_increment;
}

void NCO1_Task(void)
{
#ifdef PIR3bits
    if (PIR3bits.NCO1IF != 0)
    {
        PIR3bits.NCO1IF = 0;
    }
#endif
}
