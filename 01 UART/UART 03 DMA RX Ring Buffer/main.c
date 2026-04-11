/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for UART 03 DMA RX Ring Buffer.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "uart_dma_rx.h"

void main(void)
{
    SYSTEM_Initialize();
    UART_DMA_RX_Initialize();

    while (1)
    {
        UART_DMA_RX_Task();
    }
}
