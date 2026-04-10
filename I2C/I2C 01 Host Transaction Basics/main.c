/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for I2C 01 Host Transaction Basics.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "i2c.h"

void main(void)
{
    SYSTEM_Initialize();
    I2C1_Initialize();

    while (1)
    {
        I2C1_Task();
    }
}
