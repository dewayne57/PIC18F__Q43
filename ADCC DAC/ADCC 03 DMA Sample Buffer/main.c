/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for ADCC 03 DMA Sample Buffer.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "adcc_dma.h"

void main(void)
{
    SYSTEM_Initialize();
    ADCC_DMA_Initialize();

    while (1)
    {
        ADCC_DMA_Task();
    }
}
