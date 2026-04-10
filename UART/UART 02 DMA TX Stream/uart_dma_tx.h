/* *****************************************************************************************
 *   File Name: uart_dma_tx.h
 *   Description: DMA module API for UART 02 DMA TX Stream.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef UART_DMA_TX_H
#define UART_DMA_TX_H

#include <stdbool.h>
#include <stdint.h>

void UART_DMA_TX_Initialize(void);
void UART_DMA_TX_Task(void);

#endif /* UART_DMA_TX_H */
