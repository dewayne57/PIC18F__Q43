/* *****************************************************************************************
 *   File Name: uart_dma_tx.c
 *   Description: UART1 transmit using DMA where available, with fallback behavior.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
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

static uint8_t tx_buffers[2][UART_BUFFER_SIZE]; // Double buffers for UART transmission. Each can hold up to UART_BUFFER_SIZE bytes of data.
static volatile uint16_t tx_lengths[2];     // Current length of data in each buffer, 0 means empty and ready for new data. 
static volatile uint8_t fill_buffer_index;  // Index of the buffer currently being filled by putch(), toggles between 0 and 1.
static volatile uint8_t dma_buffer_index;   // Index of the buffer currently being transmitted by DMA, toggles between 0 and 1. 
static volatile uint8_t dma_busy;       // Flag indicating whether a DMA transfer is currently in progress. 0 = idle, 1 = busy. 

/// @brief Set the source address for DMA1 transfer.
/// @param data Pointer to the source data buffer.
/// This function configures the DMA1 source address registers to point to the provided data buffer.
/// It takes the 24-bit address of the data buffer and splits it into three 8-bit registers (SSAL, 
/// SSAH, SSAU) as required by the DMA controller.
/// @return None
static void DMA1_SetSourceAddress(const uint8_t *data)
{
    uintptr_t addr = (uintptr_t)data;

    DMA1SSAL = (uint8_t)(addr & 0xFFU);
    DMA1SSAH = (uint8_t)((addr >> 8) & 0xFFU);
    DMA1SSAU = (uint8_t)((addr >> 16) & 0xFFU);
}

/// @brief Set the destination address for DMA1 transfer.
/// This function configures the DMA1 destination address registers to point to the UART1 transmit buffer.
/// It takes the 24-bit address of the UART1 transmit buffer and splits it into three 8-bit registers (DSAL, 
/// DSAH, DSAU) as required by the DMA controller.
/// @return None
static void DMA1_SetDestAddress(void)
{
    uintptr_t addr = (uintptr_t)(&U1TXB);

    DMA1DSAL = (uint8_t)(addr & 0xFFU);
    DMA1DSAH = (uint8_t)((addr >> 8) & 0xFFU);
    DMA1DSAU = (uint8_t)((addr >> 16) & 0xFFU);
}

/// @brief Set the transfer count for DMA1 transfer.
/// @param count Number of bytes to transfer.
/// This function configures the DMA1 transfer count registers to specify how many bytes should be 
/// transferred in the current DMA operation. The count is split into two 8-bit registers (SSZL, SSZH) 
/// for the source size and (DSZL, DSZH) for the destination size. In this application, the source 
/// size is set to the count of bytes to transfer, while the destination size is fixed at 1 byte since 
/// we are writing to a UART transmit register.
/// @return None
static void DMA1_SetTransferCount(uint16_t count)
{
    DMA1SSZL = (uint8_t)(count & 0xFFU);
    DMA1SSZH = (uint8_t)((count >> 8) & 0xFFU);
    DMA1DSZL = 1U;
    DMA1DSZH = 0U;
}
/// @brief Start a DMA transfer to send data over UART.
/// @param data Pointer to the data buffer to transmit.
/// @param count Number of bytes to transmit.
/// This function initiates a DMA transfer by configuring the source address, destination address, 
/// and transfer count for DMA1. It then enables the DMA channel and starts the transfer by setting
/// the DGO bit. The function also sets the dma_busy flag to indicate that a DMA transfer is in
///progress, which prevents new transfers from being started until the current one completes.
/// @return None
static void DMA1_KickUartTx(const uint8_t *data, uint16_t count)
{
    DMA1_SetSourceAddress(data);
    DMA1_SetDestAddress();
    DMA1_SetTransferCount(count);

    DMA1CON0bits.EN = 1;
    DMA1CON0bits.DGO = 1;
    dma_busy = 1U;
}

/// @brief Service the DMA transfer for UART TX.
/// This function checks if a DMA transfer is currently in progress and if it has completed. If
/// the DMA transfer has completed (DGO bit is cleared), it resets the dma_busy flag and clears
/// the length of the buffer that was just transmitted. This function should be called from the
/// DMA interrupt service routine to handle the completion of a DMA transfer and prepare for the
/// next transfer.
/// @param None 
/// @return None
static void UART_DMA_TX_ServiceDma(void)
{
    if ((dma_busy != 0U) && (DMA1CON0bits.DGO == 0U))
    {
        dma_busy = 0U;
        tx_lengths[dma_buffer_index] = 0U;
    }
}

/// @brief Start the next DMA transfer if there is data to send and the DMA is not busy.
/// This function checks if the DMA is currently busy with a transfer. If it is not busy
/// and there is data in the buffer indicated by fill_buffer_index, it starts a new DMA 
/// transfer using the data in that buffer. It then toggles the fill_buffer_index to
/// switch to the other buffer for future data. This function is called after a DMA 
/// transfer completes to check if there is more data to send and to initiate the next 
/// transfer if needed.
/// @param None
/// @return None
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

    DMA1_KickUartTx(tx_buffers[dma_buffer], transfer_count);
}

/// @brief Initialize the UART DMA TX state.
/// This function resets the state variables used for managing the UART DMA transmission. 
/// It sets the dma_busy flag to 0 (indicating no transfer in progress), resets the 
/// fill_buffer_index and dma_buffer_index to their initial values, and clears the 
/// tx_lengths for both buffers to indicate that they are empty and ready for new data. 
/// This function should be called during system initialization to ensure that the UART 
/// DMA transmission starts with a clean state.
/// @param None 
/// @return None
void UART_DMA_TX_StateInitialize(void)
{
    dma_busy = 0U;
    fill_buffer_index = 0U;
    dma_buffer_index = 1U;
    tx_lengths[0] = 0U;
    tx_lengths[1] = 0U;
}

/// @brief Transmit a single byte via UART using DMA.
/// @param byte The byte to transmit.   
/// This function is called by the standard library's printf() function to transmit a byte
/// over UART. It attempts to place the byte into the current fill buffer. If the buffer 
/// is full, it will block until the DMA has transmitted the current buffer and made it 
/// available again. If the byte being transmitted is a newline character, it checks if
/// the previous character was a carriage return (or vice versa) to determine if it should
/// flush the buffer immediately.
/// @return None
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

        /* Buffer is full: ensure DMA runs and block until a buffer is available. */
        UART_DMA_TX_StartNext();
        INTCON0bits.GIE = gie_state;
    }
}

/// @brief DMA transmit interrupt service routine.
/// This function is called when a DMA transfer completes or encounters an error. It clears
/// the DMA interrupt flags and calls the service function to handle the completion of 
/// the DMA transfer. It then calls the function to start the next DMA transfer if there
/// is more data to send. This ISR is registered for all DMA1 interrupts (source 
///count, destination count, address error, and other error) to ensure that any condition 
/// that affects the DMA transfer will trigger the necessary handling to maintain the
/// integrity of the UART transmission process.
/// @param None
/// @return None
void UART_DMA_TX_ISR(void)
{
    DMA1SCNTIF = 0U;
    DMA1DCNTIF = 0U;
    DMA1AIF = 0U;
    DMA1ORIF = 0U;

    UART_DMA_TX_ServiceDma();
    UART_DMA_TX_StartNext();
}

/// @brief DMA1 source count interrupt service routine.
/// This function is the interrupt service routine for the DMA1 source count interrupt. 
/// It simply calls the common UART_DMA_TX_ISR() to handle the interrupt. This ISR is
/// registered to ensure that when the source count condition is met (indicating a transfer
/// has completed), the appropriate handling is performed to manage the DMA state and
/// initiate the next transfer if needed.
/// @param None
/// @return None
void __interrupt(irq(IRQ_DMA1SCNT), low_priority) DMA1_SCNT_ISR(void)
{
    UART_DMA_TX_ISR();
}

/// @brief DMA1 destination count interrupt service routine.
/// This function is the interrupt service routine for the DMA1 destination count interrupt. 
/// It simply calls the common UART_DMA_TX_ISR() to handle the interrupt. This ISR is
/// registered to ensure that when the destination count condition is met (indicating a transfer
/// has completed), the appropriate handling is performed to manage the DMA state and
/// initiate the next transfer if needed.
/// @param None
/// @return None
void __interrupt(irq(IRQ_DMA1DCNT), low_priority) DMA1_DCNT_ISR(void)
{
    UART_DMA_TX_ISR();
}

/// @brief DMA1 other error interrupt service routine.
/// This function is the interrupt service routine for the DMA1 other error interrupt. 
/// It simply calls the common UART_DMA_TX_ISR() to handle the interrupt. This ISR is
/// registered to ensure that when the other error condition is met (indicating a transfer
/// has encountered an error), the appropriate handling is performed to manage the DMA state and
/// initiate the next transfer if needed.
/// @param None
/// @return None
void __interrupt(irq(IRQ_DMA1OR), low_priority) DMA1_OR_ISR(void)
{
    UART_DMA_TX_ISR();
}

/// @brief DMA1 address error interrupt service routine.
/// This function is the interrupt service routine for the DMA1 address error interrupt.    
/// It simply calls the common UART_DMA_TX_ISR() to handle the interrupt. This ISR is
/// registered to ensure that when the address error condition is met (indicating a transfer
/// has encountered an error), the appropriate handling is performed to manage the DMA state and
/// initiate the next transfer if needed.
/// @param None
/// @return None
void __interrupt(irq(IRQ_DMA1A), low_priority) DMA1_A_ISR(void)
{
    UART_DMA_TX_ISR();
}
