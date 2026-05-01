/* *****************************************************************************************
 *   File Name: uart_dma_rx.c
 *   Description: UART1 receive using DMA ping-pong buffers.
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
#include "uart_dma_rx.h"

#define UART_DMA_RX_INDEX_INVALID (0xFFU)

static uint8_t rx_buffers[2U][UART_BUFFER_SIZE];
static volatile uint16_t rx_lengths[2U];
static volatile uint8_t dma_fill_buffer_index;
static volatile uint8_t checked_out_buffer_index;
static volatile uint8_t dma_busy;
static volatile uint8_t rx_paused;

/// @brief Assert or deassert the RTS output to signal flow-control state to the remote sender.
/// @param ready true to assert ready (remote may send), false to deassert (remote must pause).
static void UART_DMA_RX_SetFlowControlReady(bool ready)
{
    UART_RTS_LAT = (ready ? UART_RTS_READY_LEVEL : UART_RTS_PAUSE_LEVEL);
}

/// @brief Load the DMA1 source address registers with the address of the UART1 receive register.
/// The 24-bit address of U1RXB is split across the SSAL, SSAH, and SSAU registers.
static void DMA1_SetSourceAddress(void)
{
    uintptr_t addr = (uintptr_t)(&U1RXB);

    DMA1SSAL = (uint8_t)(addr & 0xFFU);
    DMA1SSAH = (uint8_t)((addr >> 8) & 0xFFU);
    DMA1SSAU = (uint8_t)((addr >> 16) & 0xFFU);
}

/// @brief Load the DMA1 destination address registers with the address of an RX buffer.
/// @param data Pointer to the start of the destination buffer in data memory.
static void DMA1_SetDestAddress(uint8_t *data)
{
    uintptr_t addr = (uintptr_t)data;

    DMA1DSAL = (uint8_t)(addr & 0xFFU);
    DMA1DSAH = (uint8_t)((addr >> 8) & 0xFFU);
    DMA1DSAU = (uint8_t)((addr >> 16) & 0xFFU);
}

/// @brief Configure the DMA1 source and destination transfer sizes for one RX capture.
/// The source size is fixed at 1 (single UART RX register) and the destination size is
/// set to @p count so DMA fills the entire destination buffer before interrupting.
/// @param count Number of bytes to receive into the destination buffer.
static void DMA1_SetTransferCount(uint16_t count)
{
    DMA1SSZL = 1U;
    DMA1SSZH = 0U;
    DMA1DSZL = (uint8_t)(count & 0xFFU);
    DMA1DSZH = (uint8_t)((count >> 8) & 0xFFU);
}

/// @brief Arm DMA1 to receive UART_BUFFER_SIZE bytes into the specified RX buffer.
/// Configures source, destination, and count registers, marks the buffer as filling,
/// asserts RTS ready, clears the paused flag, and starts the DMA transfer.
/// @param buffer_index Index of the ping-pong buffer to fill (0 or 1).
static void UART_DMA_RX_StartCapture(uint8_t buffer_index)
{
    DMA1_SetSourceAddress();
    DMA1_SetDestAddress(rx_buffers[buffer_index]);
    DMA1_SetTransferCount(UART_BUFFER_SIZE);

    dma_fill_buffer_index = buffer_index;
    dma_busy = 1U;
    UART_DMA_RX_SetFlowControlReady(true);
    rx_paused = 0U;

    DMA1CON0bits.EN = 1U;
    DMA1CON0bits.DGO = 1U;
}

/// @brief Return the index of a ping-pong buffer that is free for DMA capture.
/// A buffer is considered free when its length is zero and it is not currently
/// checked out by the application. Returns UART_DMA_RX_INDEX_INVALID when both
/// buffers are occupied.
/// @return 0, 1, or UART_DMA_RX_INDEX_INVALID.
static uint8_t UART_DMA_RX_FindFreeBuffer(void)
{
    if ((rx_lengths[0U] == 0U) && (checked_out_buffer_index != 0U))
    {
        return 0U;
    }

    if ((rx_lengths[1U] == 0U) && (checked_out_buffer_index != 1U))
    {
        return 1U;
    }

    return UART_DMA_RX_INDEX_INVALID;
}

/// @brief Start the next DMA capture if the DMA channel is idle and a free buffer exists.
/// If the channel is already busy the function returns immediately. If no free buffer
/// is available the DMA channel is disabled, RTS is deasserted, and rx_paused is set
/// so that ReleaseBuffer() can resume capture once space is available.
static void UART_DMA_RX_StartNextIfIdle(void)
{
    uint8_t next_buffer;

    if (dma_busy != 0U)
    {
        return;
    }

    next_buffer = UART_DMA_RX_FindFreeBuffer();
    if (next_buffer == UART_DMA_RX_INDEX_INVALID)
    {
        DMA1CON0bits.EN = 0U;
        UART_DMA_RX_SetFlowControlReady(false);
        rx_paused = 1U;
        return;
    }

    UART_DMA_RX_StartCapture(next_buffer);
}

/// @brief Called from the DMA ISR to handle a completed receive transfer.
/// Returns immediately if DMA is not busy or the DGO bit is still set.
/// On completion, records the filled buffer length, clears the fill-index,
/// and attempts to start capture into the other buffer.
static void UART_DMA_RX_ServiceDma(void)
{
    uint8_t completed_buffer;

    if ((dma_busy == 0U) || (DMA1CON0bits.DGO != 0U))
    {
        return;
    }

    completed_buffer = dma_fill_buffer_index;
    dma_busy = 0U;
    dma_fill_buffer_index = UART_DMA_RX_INDEX_INVALID;

    rx_lengths[completed_buffer] = UART_BUFFER_SIZE;

    UART_DMA_RX_StartNextIfIdle();
}

/// @brief Initialize all RX DMA state variables and start the first capture.
/// Disables the DMA channel, clears both buffer lengths, resets index tracking
/// variables, asserts RTS ready, and arms capture into buffer 0. Call this once
/// during system initialization, with global interrupts disabled.
void UART_DMA_RX_StateInitialize(void)
{
    DMA1CON0bits.EN = 0U;
    DMA1CON0bits.DGO = 0U;

    rx_lengths[0U] = 0U;
    rx_lengths[1U] = 0U;

    dma_fill_buffer_index = UART_DMA_RX_INDEX_INVALID;
    checked_out_buffer_index = UART_DMA_RX_INDEX_INVALID;
    dma_busy = 0U;
    rx_paused = 0U;

    UART_DMA_RX_SetFlowControlReady(true);

    UART_DMA_RX_StartCapture(0U);
}

/// @brief Return true if at least one RX buffer is ready for the application to read.
/// Returns true if a buffer is currently checked out or if either buffer contains
/// data captured by DMA. This function is safe to call from the main loop without
/// disabling interrupts.
/// @return true if data is available, false otherwise.
bool UART_DMA_RX_BufferAvailable(void)
{
    if (checked_out_buffer_index != UART_DMA_RX_INDEX_INVALID)
    {
        return true;
    }

    if ((rx_lengths[0U] != 0U) || (rx_lengths[1U] != 0U))
    {
        return true;
    }

    return false;
}

/// @brief Retrieve a pointer to a completed RX buffer and report its byte count.
/// If a buffer is already checked out, returns it again without advancing state.
/// Otherwise promotes the first buffer with a non-zero length to the checked-out
/// slot. Returns NULL when no completed buffer is available. The returned pointer
/// is valid until UART_DMA_RX_ReleaseBuffer() is called.
/// @param length Output parameter set to the number of valid bytes in the buffer.
///               Set to 0 on error. May be NULL if the count is not needed.
/// @return Pointer to the receive buffer, or NULL if none is ready.
const uint8_t *UART_DMA_RX_GetBuffer(uint16_t *length)
{
    uint8_t gie_state;
    uint8_t ready_buffer = UART_DMA_RX_INDEX_INVALID;

    if (length != NULL)
    {
        *length = 0U;
    }

    gie_state = INTCON0bits.GIE;
    INTCON0bits.GIE = 0U;

    if (checked_out_buffer_index != UART_DMA_RX_INDEX_INVALID)
    {
        if (length != NULL)
        {
            *length = rx_lengths[checked_out_buffer_index];
        }

        INTCON0bits.GIE = gie_state;
        return rx_buffers[checked_out_buffer_index];
    }

    if (rx_lengths[0U] != 0U)
    {
        ready_buffer = 0U;
    }
    else if (rx_lengths[1U] != 0U)
    {
        ready_buffer = 1U;
    }

    if (ready_buffer == UART_DMA_RX_INDEX_INVALID)
    {
        INTCON0bits.GIE = gie_state;
        return NULL;
    }

    checked_out_buffer_index = ready_buffer;

    if (length != NULL)
    {
        *length = rx_lengths[ready_buffer];
    }

    INTCON0bits.GIE = gie_state;

    return rx_buffers[ready_buffer];
}

/// @brief Return the checked-out RX buffer to the DMA capture pool.
/// Clears the buffer length, clears the checked-out index, and, if DMA capture
/// was previously paused due to no free buffers, resumes capture immediately.
/// Must be called after the application has finished reading the data returned
/// by UART_DMA_RX_GetBuffer().
void UART_DMA_RX_ReleaseBuffer(void)
{
    uint8_t gie_state;

    gie_state = INTCON0bits.GIE;
    INTCON0bits.GIE = 0U;

    if (checked_out_buffer_index != UART_DMA_RX_INDEX_INVALID)
    {
        rx_lengths[checked_out_buffer_index] = 0U;
        checked_out_buffer_index = UART_DMA_RX_INDEX_INVALID;
    }

    if (rx_paused != 0U)
    {
        UART_DMA_RX_StartNextIfIdle();
    }

    INTCON0bits.GIE = gie_state;
}

/// @brief Common DMA1 RX interrupt service handler.
/// Clears all DMA1 interrupt flags, then calls UART_DMA_RX_ServiceDma() to
/// finalize the completed transfer and arm the next capture. This function is
/// invoked by each of the four DMA1 peripheral ISRs.
void UART_DMA_RX_ISR(void)
{
    DMA1SCNTIF = 0U;
    DMA1DCNTIF = 0U;
    DMA1AIF = 0U;
    DMA1ORIF = 0U;

    UART_DMA_RX_ServiceDma();
}

/// @brief DMA1 source-count interrupt — fired when DMA1 has read all source bytes.
void __interrupt(irq(IRQ_DMA1SCNT), low_priority) DMA1_SCNT_ISR(void)
{
    UART_DMA_RX_ISR();
}

/// @brief DMA1 destination-count interrupt — fired when DMA1 has filled the destination buffer.
void __interrupt(irq(IRQ_DMA1DCNT), low_priority) DMA1_DCNT_ISR(void)
{
    UART_DMA_RX_ISR();
}

/// @brief DMA1 overrun/other-error interrupt — fired when DMA1 encounters a bus error.
void __interrupt(irq(IRQ_DMA1OR), low_priority) DMA1_OR_ISR(void)
{
    UART_DMA_RX_ISR();
}

/// @brief DMA1 address-error interrupt — fired when DMA1 accesses an invalid address.
void __interrupt(irq(IRQ_DMA1A), low_priority) DMA1_A_ISR(void)
{
    UART_DMA_RX_ISR();
}
