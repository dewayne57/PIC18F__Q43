/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for PWM 01 Edge Aligned Duty Control.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "pwm.h"

void main(void)
{
    SYSTEM_Initialize();
    PWM1_Initialize();

    while (1)
    {
        PWM1_Task();
    }
}
