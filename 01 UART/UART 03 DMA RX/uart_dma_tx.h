/* *****************************************************************************************
 *   File Name: uart_dma_tx.h
 *   Description: DMA module API for UART TX in UART 03 DMA RX.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-01
 *
 *   Copyright (c) 2026, Dewayne Hafenstein.
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 * ***************************************************************************************** */

#ifndef UART_DMA_TX_H
#define UART_DMA_TX_H

#include <stdbool.h>
#include <stdint.h>

void UART_DMA_TX_ISR(void);
void putch(uint8_t byte);
void UART_DMA_TX_StateInitialize(void);

#endif /* UART_DMA_TX_H */
