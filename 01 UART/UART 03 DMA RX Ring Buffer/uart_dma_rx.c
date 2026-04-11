/* *****************************************************************************************
 *   File Name: uart_dma_rx.c
 *   Description: DMA scaffold for UART 03 DMA RX Ring Buffer.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "uart_dma_rx.h"

static volatile uint8_t s_dma_busy;
static volatile uint16_t s_dma_events;

void UART_DMA_RX_Initialize(void)
{
    s_dma_busy = 0U;
    s_dma_events = 0U;

    /* TODO: Configure DMA source, destination, trigger, and transfer count. */
}

void UART_DMA_RX_Task(void)
{
    if (s_dma_busy == 0U)
    {
        /* TODO: Queue next DMA transaction for this peripheral demo. */
        s_dma_busy = 1U;
    }

    if (s_dma_busy != 0U)
    {
        /* Placeholder completion pulse for early bring-up visibility. */
        s_dma_busy = 0U;
        s_dma_events++;

        if ((s_dma_events & 0x001FU) == 0U)
        {
            LATDbits.LATD0 ^= 1U;
        }
    }
}
