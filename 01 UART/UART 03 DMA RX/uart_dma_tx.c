/* *****************************************************************************************
 *   File Name: uart_dma_tx.c
 *   Description: UART1 transmit using DMA2 with ping-pong TX buffers.
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

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "config.h"
#include "uart_dma_tx.h"

static uint8_t tx_buffers[2][UART_BUFFER_SIZE];
static volatile uint16_t tx_lengths[2];
static volatile uint8_t fill_buffer_index;
static volatile uint8_t dma_buffer_index;
static volatile uint8_t dma_busy;

static void DMA2_SetSourceAddress(const uint8_t *data)
{
    uintptr_t addr = (uintptr_t)data;

    DMA2SSAL = (uint8_t)(addr & 0xFFU);
    DMA2SSAH = (uint8_t)((addr >> 8) & 0xFFU);
    DMA2SSAU = (uint8_t)((addr >> 16) & 0xFFU);
}

static void DMA2_SetDestAddress(void)
{
    uintptr_t addr = (uintptr_t)(&U1TXB);

    DMA2DSAL = (uint8_t)(addr & 0xFFU);
    DMA2DSAH = (uint8_t)((addr >> 8) & 0xFFU);
    DMA2DSAU = (uint8_t)((addr >> 16) & 0xFFU);
}

static void DMA2_SetTransferCount(uint16_t count)
{
    DMA2SSZL = (uint8_t)(count & 0xFFU);
    DMA2SSZH = (uint8_t)((count >> 8) & 0xFFU);
    DMA2DSZL = 1U;
    DMA2DSZH = 0U;
}

static void DMA2_KickUartTx(const uint8_t *data, uint16_t count)
{
    DMA2_SetSourceAddress(data);
    DMA2_SetDestAddress();
    DMA2_SetTransferCount(count);

    DMA2CON0bits.EN = 1U;
    DMA2CON0bits.DGO = 1U;
    dma_busy = 1U;
}

static void UART_DMA_TX_ServiceDma(void)
{
    if ((dma_busy != 0U) && (DMA2CON0bits.DGO == 0U))
    {
        dma_busy = 0U;
        tx_lengths[dma_buffer_index] = 0U;
    }
}

static void UART_DMA_TX_StartNext(void)
{
    uint8_t dma_buffer;
    uint16_t transfer_count;

    if (dma_busy != 0U)
    {
        return;
    }

    if (tx_lengths[fill_buffer_index] == 0U)
    {
        return;
    }

    dma_buffer = fill_buffer_index;
    fill_buffer_index ^= 1U;
    dma_buffer_index = dma_buffer;
    transfer_count = tx_lengths[dma_buffer];

    DMA2_KickUartTx(tx_buffers[dma_buffer], transfer_count);
}

void UART_DMA_TX_StateInitialize(void)
{
    dma_busy = 0U;
    fill_buffer_index = 0U;
    dma_buffer_index = 1U;
    tx_lengths[0] = 0U;
    tx_lengths[1] = 0U;
}

void putch(uint8_t byte)
{
    uint8_t fill_buffer;
    uint16_t fill_length;
    bool flush_now;
    uint8_t gie_state;

    for (;;)
    {
        gie_state = INTCON0bits.GIE;
        INTCON0bits.GIE = 0U;

        fill_buffer = fill_buffer_index;
        fill_length = tx_lengths[fill_buffer];

        if (fill_length < UART_BUFFER_SIZE)
        {
            tx_buffers[fill_buffer][fill_length] = byte;
            tx_lengths[fill_buffer] = (uint16_t)(fill_length + 1U);

            flush_now = false;
            if (fill_length != 0U)
            {
                uint8_t previous = tx_buffers[fill_buffer][fill_length - 1U];
                if (((previous == '\r') && (byte == '\n')) ||
                    ((previous == '\n') && (byte == '\r')))
                {
                    flush_now = true;
                }
            }

            if (flush_now)
            {
                UART_DMA_TX_StartNext();
            }

            INTCON0bits.GIE = gie_state;
            break;
        }

        UART_DMA_TX_StartNext();
        INTCON0bits.GIE = gie_state;
    }
}

void UART_DMA_TX_ISR(void)
{
    DMA2SCNTIF = 0U;
    DMA2DCNTIF = 0U;
    DMA2AIF = 0U;
    DMA2ORIF = 0U;

    UART_DMA_TX_ServiceDma();
    UART_DMA_TX_StartNext();
}

void __interrupt(irq(IRQ_DMA2SCNT), low_priority) DMA2_SCNT_ISR(void)
{
    UART_DMA_TX_ISR();
}

void __interrupt(irq(IRQ_DMA2DCNT), low_priority) DMA2_DCNT_ISR(void)
{
    UART_DMA_TX_ISR();
}

void __interrupt(irq(IRQ_DMA2OR), low_priority) DMA2_OR_ISR(void)
{
    UART_DMA_TX_ISR();
}

void __interrupt(irq(IRQ_DMA2A), low_priority) DMA2_A_ISR(void)
{
    UART_DMA_TX_ISR();
}
