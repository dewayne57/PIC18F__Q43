/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for UART 02 DMA TX Stream.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "uart_dma_tx.h"

void main(void)
{
    SYSTEM_Initialize();
    UART_DMA_TX_Initialize();

    while (1)
    {
        UART_DMA_TX_Task();
    }
}
