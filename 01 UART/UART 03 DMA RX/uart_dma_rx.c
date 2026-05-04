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
 *
 *   This module implements UART receive using DMA with ping-pong buffering. Two buffers
 *   are allocated in data memory, and DMA is configured to fill one buffer at a time with
 *   incoming UART data. When a buffer is full, the DMA interrupt service routine marks it as
 *   ready for the application to read and starts filling the other buffer. The application
 *   can check for available data and retrieve completed buffers through the API functions
 *   defined in uart_dma_rx.h. This design allows continuous reception of UART data with
 *   minimal CPU intervention, as the DMA handles all data transfers and buffering. The main
 *   application can focus on processing received data without worrying about the timing of UART
 *   reception, making it ideal for high-throughput or time-sensitive applications.
 *
 *   The DMA1 channel is used for UART RX capture, while the DMA2 channel is reserved for UART
 *   TX streaming (implemented in uart_dma_tx.c). This separation allows simultaneous RX and TX
 *   operations without resource conflicts. The DMA RX module manages its own state and buffer
 *   tracking, ensuring that the application can safely retrieve completed buffers without
 *   needing to manage DMA state directly. The API functions handle synchronization and
 *   state transitions, providing a clean interface for the main application to interact
 *   with the DMA RX functionality.
 *
 *   The consequence of this design is that the application must call
 *   UART_DMA_RX_ReleaseBuffer() after it is done processing a received buffer to return
 *   it to the pool of available buffers for DMA capture. If the application fails to
 *   release buffers, the DMA will eventually run out of free buffers and pause reception
 *   until a buffer is released. However, as long as the application processes and releases
 *   buffers in a timely manner, this design allows for efficient and continuous UART
 *   reception with minimal CPU overhead.
 *
 *   Also, this design is essentially Line-Oriented, as the application is expected to
 *   process complete buffers of data, which may correspond to lines of text or fixed-size
 *   packets, rather than individual bytes. The DMA fills buffers with incoming data, and
 *   the application retrieves and processes whole buffers at a time.  This may not be
 *   ideal for all applications, especially those that require byte-by-byte processing or
 *   low-latency handling of incoming data. However, for many use cases where data is
 *   naturally buffered or packetized, this design provides an efficient way to handle
 *   UART reception with DMA.  Ensure that the buffer size and processing logic in the
 *   application are designed to work well with this buffered approach to avoid issues
 *   with latency or buffer overflows.
 *
 *   This code reads input lines terminated by CR+LF or LF+CR, or when the buffer fills
 *   up. The application can process the received lines at its own pace.  It must call
 *   UART_DMA_RX_GetBuffer() to retrieve a completed buffer.  If no buffer is currently
 *   available the function will return NULL.  When the buffer has been processed the
 *   application must call UART_DMA_RX_ReleaseBuffer() to return it to the pool of
 *   available buffers.
 *
 *   Different line termination schemes or packet formats can be implemented by modifying 
 *   the UART_DMA_RX_ServiceDma() function to detect the desired conditions for marking a
 *   buffer as complete.  For example, if the message to be received has a fixed
 *   length, the service routine could mark the buffer as complete when that length is
 *   reached, regardless of the content.  Another common scheme is to include the
 *   message length as a header in the data stream, and the service routine could read
 *   the length from the header to determine when the buffer is complete.  Other
 *   variations could include using a specific delimiter character, or a timeout
 *   mechanism to mark a buffer as complete if no new data is received for a certain
 *   period of time. The current implementation is line-oriented for simplicity, but
 *   the design can be adapted to different data formats and application requirements
 *   by modifying the logic in the DMA service routine.
 *
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
static volatile uint16_t dma_fill_length;
static volatile uint8_t dma_prev_byte;
static volatile uint8_t dma_prev_valid;

/// @brief Load the DMA1 source address registers with the address of the UART1
/// receive register.  The source address is fixed for UART RX, so this function
/// always loads the same address. The 24-bit address of U1RXB is split across
/// the SSAL, SSAH, and SSAU registers.  /// This function is called by
/// UART_DMA_RX_StartCapture() to set the source address before starting each
/// DMA capture.
/// @param None
/// @return None
static void DMA1_SetSourceAddress(void)
{
    uintptr_t addr = (uintptr_t)(&U1RXB);

    DMA1SSAL = (uint8_t)(addr & 0xFFU);
    DMA1SSAH = (uint8_t)((addr >> 8) & 0xFFU);
    DMA1SSAU = (uint8_t)((addr >> 16) & 0xFFU);
}

/// @brief Load the DMA1 destination address registers with the address of an
/// RX buffer. The destination address is set to the start of one of the two
/// ping-pong buffers allocated in data memory. The 24-bit address of the
/// selected buffer is split across the DSAL, DSAH, and DSAU registers. This
/// function is called by UART_DMA_RX_StartCapture() to set the destination
/// address before starting each DMA capture.
/// @param data Pointer to the start of the destination buffer in data memory.
/// @return None
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

/// @brief Arm DMA1 to receive one byte into the specified RX buffer and offset.
/// @param buffer_index Index of the ping-pong buffer to fill (0 or 1).
/// @param offset Byte offset within the selected buffer.
static void UART_DMA_RX_StartCaptureByte(uint8_t buffer_index, uint16_t offset)
{
    DMA1_SetSourceAddress();
    DMA1_SetDestAddress(&rx_buffers[buffer_index][offset]);
    DMA1_SetTransferCount(1U);

    dma_fill_buffer_index = buffer_index;
    dma_busy = 1U;
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

/// @brief Start capture of a new line if DMA is idle and a free buffer exists.
/// If the channel is already busy the function returns immediately. If no free buffer
/// is available the DMA channel is disabled and rx_paused is set
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
        rx_paused = 1U;
        return;
    }

    dma_fill_length = 0U;
    dma_prev_valid = 0U;
    UART_DMA_RX_StartCaptureByte(next_buffer, 0U);
}

/// @brief Called from the DMA ISR to handle each completed received byte.
/// A line is published when CR+LF or LF+CR is detected, or when the active buffer
/// reaches UART_BUFFER_SIZE. Otherwise DMA is immediately re-armed for the next byte.
/// @param None
/// @return None
static void UART_DMA_RX_ServiceDma(void)
{
    uint8_t active_buffer;
    uint8_t current_byte;
    uint16_t next_length;
    uint8_t line_complete = 0U;

    if ((dma_busy == 0U) || (DMA1CON0bits.DGO != 0U))
    {
        return;
    }

    active_buffer = dma_fill_buffer_index;
    current_byte = rx_buffers[active_buffer][dma_fill_length];
    next_length = (uint16_t)(dma_fill_length + 1U);

    if ((dma_prev_valid != 0U) &&
        (((dma_prev_byte == '\r') && (current_byte == '\n')) ||
         ((dma_prev_byte == '\n') && (current_byte == '\r'))))
    {
        line_complete = 1U;
    }

    if (next_length >= UART_BUFFER_SIZE)
    {
        line_complete = 1U;
    }

    dma_fill_length = next_length;

    if (line_complete == 0U)
    {
        dma_prev_byte = current_byte;
        dma_prev_valid = 1U;
        UART_DMA_RX_StartCaptureByte(active_buffer, dma_fill_length);
        return;
    }

    dma_busy = 0U;
    dma_fill_buffer_index = UART_DMA_RX_INDEX_INVALID;

    rx_lengths[active_buffer] = dma_fill_length;

    UART_DMA_RX_StartNextIfIdle();
}

/// @brief Initialize all RX DMA state variables and start the first capture.
/// Disables the DMA channel, clears both buffer lengths, resets index tracking
/// variables, and arms capture into buffer 0. Call this once
/// during system initialization, with global interrupts disabled.
/// @param None
/// @return None
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
    dma_fill_length = 0U;
    dma_prev_byte = 0U;
    dma_prev_valid = 0U;

    UART_DMA_RX_StartNextIfIdle();
}

/// @brief Return true if at least one RX buffer is ready for the application to read.
/// Returns true if a buffer is currently checked out or if either buffer contains
/// data captured by DMA. This function is safe to call from the main loop without
/// disabling interrupts.
/// @param None
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
/// @param None
/// @return None
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
/// @param None
/// @return None
void UART_DMA_RX_ISR(void)
{
    DMA1SCNTIF = 0U;
    DMA1DCNTIF = 0U;
    DMA1AIF = 0U;
    DMA1ORIF = 0U;

    UART_DMA_RX_ServiceDma();
}

/// @brief DMA1 source-count interrupt — fired when DMA1 has read all source bytes.
/// @param None
/// @return None
void __interrupt(irq(IRQ_DMA1SCNT), low_priority) DMA1_SCNT_ISR(void)
{
    UART_DMA_RX_ISR();
}

/// @brief DMA1 destination-count interrupt — fired when DMA1 has filled the destination buffer.
/// @param None
/// @return None
void __interrupt(irq(IRQ_DMA1DCNT), low_priority) DMA1_DCNT_ISR(void)
{
    UART_DMA_RX_ISR();
}

/// @brief DMA1 overrun/other-error interrupt — fired when DMA1 encounters a bus error.
/// @param None
/// @return None
void __interrupt(irq(IRQ_DMA1OR), low_priority) DMA1_OR_ISR(void)
{
    UART_DMA_RX_ISR();
}

/// @brief DMA1 address-error interrupt — fired when DMA1 accesses an invalid address.
/// @param None
/// @return None
void __interrupt(irq(IRQ_DMA1A), low_priority) DMA1_A_ISR(void)
{
    UART_DMA_RX_ISR();
}
