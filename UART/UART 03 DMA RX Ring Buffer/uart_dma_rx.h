/* *****************************************************************************************
 *   File Name: uart_dma_rx.h
 *   Description: DMA module API for UART 03 DMA RX Ring Buffer.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef UART_DMA_RX_H
#define UART_DMA_RX_H

#include <stdbool.h>
#include <stdint.h>

void UART_DMA_RX_Initialize(void);
void UART_DMA_RX_Task(void);

#endif /* UART_DMA_RX_H */
