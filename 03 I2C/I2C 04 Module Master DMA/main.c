/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for I2C 04 Module Master DMA.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "i2c_dma.h"

void main(void)
{
    SYSTEM_Initialize();
    I2C_DMA_Initialize();

    while (1)
    {
        I2C_DMA_Task();
    }
}
