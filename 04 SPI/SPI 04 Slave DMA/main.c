/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for SPI 02 DMA Burst Transfer.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "spi_dma.h"

void main(void)
{
    SYSTEM_Initialize();
    SPI_DMA_Initialize();

    while (1)
    {
        SPI_DMA_Task();
    }
}
