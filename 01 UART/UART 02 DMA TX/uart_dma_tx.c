/* *****************************************************************************************
 *   File Name: uart_dma_tx.c
 *   Description: UART1 transmit using DMA1 with ping-pong buffers and recovery fallback.
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
#include "dma.h"
#include "uart_dma_tx.h"

/* Ping-pong double-buffer arrangement:
   Two equal-sized TX buffers allow one buffer to be filled by the application ("fill buffer")
   while DMA is simultaneously draining the other buffer ("DMA buffer") to UART TX.
   When DMA finishes, the roles swap: the newly filled buffer becomes the DMA buffer and
   the just-emptied one becomes available for the application to refill.
   This overlap of filling and sending removes the need to wait for each transfer to complete
   before queuing the next one. */
static uint8_t tx_buffers[2][UART_BUFFER_SIZE]; // Two transmit buffers (index 0 and 1)
static volatile uint16_t tx_lengths[2];          // Number of valid bytes in each buffer
static volatile uint8_t fill_buffer_index;       // Which buffer the application is currently filling
static volatile uint8_t dma_buffer_index;        // Which buffer DMA is currently transmitting (INVALID = idle)
static volatile uint16_t dma_stall_counter;      // Incremented each service call if DMA appears stalled

#define UART_DMA_TX_INDEX_INVALID 0xFFU     // Sentinel: no DMA buffer currently active
#define DMA_STALL_RECOVERY_THRESHOLD 20U    // Service calls without DMA progress before recovery
#define UART_TXIF_WAIT_TIMEOUT 60000U       // Polling iterations before giving up on TX-ready

/// @brief Send a block of bytes directly through UART TX without DMA.
/// @param data Pointer to source bytes.
/// @param count Number of bytes to send.
/// @return None
/// @note This is a fallback path used only during DMA error recovery.  Under
///       normal operation data is always sent via DMA; blocking UART writes are
///       used here to flush any bytes stranded in a buffer that DMA could not finish.
static void UART_TX_SendBlocking(const uint8_t *data, uint16_t count)
{
    uint16_t i;

    for (i = 0U; i < count; i++)
    {
        /* Poll until UART TX FIFO has room, then write the next byte. */
        while (PIR4bits.U1TXIF == 0U)
        {
        }
        U1TXB = data[i];
    }
}

/// @brief Abort current DMA transfer and flush pending bytes through UART TX.
/// @param None
/// @return None
static void UART_DMA_TX_AbortAndRecover(void)
{
    uint8_t active_buffer;
    uint16_t active_length;

    active_buffer = dma_buffer_index;
    if (active_buffer == UART_DMA_TX_INDEX_INVALID)
    {
        return;
    }

    active_length = tx_lengths[active_buffer];

    DMA_SelectChannel(1U);
    DMAnCON0bits.DGO = 0U;
    DMAnCON0bits.EN = 0U;

    if (active_length != 0U)
    {
        /* Recovery path: flush the buffer with direct UART writes. */
        UART_TX_SendBlocking(tx_buffers[active_buffer], active_length);
        tx_lengths[active_buffer] = 0U;
    }

    dma_buffer_index = UART_DMA_TX_INDEX_INVALID;
    dma_stall_counter = 0U;
}

/// @brief Prime one UART byte and start DMA1 for the remainder.
static void DMA1_KickUartTx(volatile uint8_t *data, uint16_t count)
{
    uint16_t txif_wait;

    if (count == 0U)
    {
        return;
    }

    /* The PIC18 DMA engine requires an active hardware trigger to move each byte.
       UART1 only asserts its TX trigger when its hardware FIFO has space.  If we
       start DMA immediately (without priming the FIFO first) the trigger may not
       fire at all because the FIFO has not yet seen any demand.

       Fix: manually write the FIRST byte directly to U1TXB here.  This is guaranteed
       to work (TXIF will assert within a few cycles once the module is enabled), and
       it gets the UART TX shift register busy so that subsequent trigger pulses arrive
       regularly as each byte is shifted out. */
    txif_wait = 0U;
    while (PIR4bits.U1TXIF == 0U)
    {
        if (txif_wait >= UART_TXIF_WAIT_TIMEOUT)
        {
            /* Timeout guard: if the UART is not accepting data, abort this transfer
               to avoid an infinite loop hanging the application. */
            if (dma_buffer_index != UART_DMA_TX_INDEX_INVALID)
            {
                tx_lengths[dma_buffer_index] = 0U;
                dma_buffer_index = UART_DMA_TX_INDEX_INVALID;
            }
            return;
        }
        txif_wait++;
    }
    U1TXB = data[0U]; // Prime the UART TX FIFO with the first byte

    if (count == 1U)
    {
        /* Only one byte - DMA is not needed; mark the transfer done. */
        tx_lengths[dma_buffer_index] = 0U;
        dma_buffer_index = UART_DMA_TX_INDEX_INVALID;
        return;
    }

    /* Point DMA source to byte[1] (byte[0] was already sent above) and
       set the remaining count.  The destination is always the UART TX register. */
    data = &data[1U];
    count = (uint16_t)(count - 1U);

    DMA_SetSourceAddress(1U, data);
    DMA_SetDestAddress(1U, &U1TXB);
    DMA_SetTransferCount(1U, count);

    /* Configure and start the DMA channel:
       DGO=0 first to ensure a clean start state.
       EN=0 then EN=1 resets any pending state in the DMA arbiter.
       SIRQEN=1 allows the hardware trigger (UART1 TX FIFO space) to advance the DMA.
       AIRQEN=0 disables abort-trigger input.
       DGO=1 starts the transfer - DMA will now move bytes on each trigger pulse. */
    DMA_SelectChannel(1U);
    DMAnCON0bits.DGO = 0U;    // Ensure DMA is not already running
    DMAnCON0bits.EN = 0U;
    DMAnCON0bits.SIRQEN = 1U; // Enable UART1 TX trigger to advance DMA
    DMAnCON0bits.AIRQEN = 0U;
    DMAnCON0bits.EN = 1U;     // Enable the DMA channel
    DMAnCON0bits.DGO = 1U;    // Start the transfer (DMA Go)
}

/// @brief Service completion/error flags and recover stalled DMA transfers.
/// @param None
/// @return None
static void UART_DMA_TX_ServiceDma(void)
{
    /* SCNTIF/DCNTIF: normal completion flags - transfer finished successfully. Clear and
       reset the stall counter since we have confirmed DMA is making progress. */
    if ((DMA1SCNTIF != 0U) || (DMA1DCNTIF != 0U))
    {
        DMA1SCNTIF = 0U;
        DMA1DCNTIF = 0U;
        dma_stall_counter = 0U;  // Evidence of progress, reset watchdog
    }

    /* AIF/ORIF: error flags - DMA was aborted or the UART overflowed. Trigger recovery. */
    if ((DMA1AIF != 0U) || (DMA1ORIF != 0U))
    {
        DMA1AIF = 0U;
        DMA1ORIF = 0U;
        UART_DMA_TX_AbortAndRecover();  // Flush the stuck buffer via blocking UART writes
        return;
    }

    /* Stall detection: if DMA says it is busy (DGO=1) but no completion flags have
       cleared recently, increment a counter.  If it exceeds the threshold, assume
       the DMA is wedged and force a recovery.  This handles the rare case where the
       completion ISR is missed or the DMA arbiter gets stuck. */
    if ((dma_buffer_index != UART_DMA_TX_INDEX_INVALID) && DMA_IsTransferInProgress(1U))
    {
        if (dma_stall_counter < DMA_STALL_RECOVERY_THRESHOLD)
        {
            dma_stall_counter++;  // Another call with no progress - increment watchdog
        }
        else
        {
            UART_DMA_TX_AbortAndRecover();  // Stall threshold exceeded - force recovery
            return;
        }
    }
    else
    {
        dma_stall_counter = 0U;  // DMA is idle or not ours - no stall
    }

    /* If our DMA buffer owner flag is set but DMA has stopped, release the buffer. */
    if ((dma_buffer_index != UART_DMA_TX_INDEX_INVALID) && !DMA_IsTransferInProgress(1U))
    {
        tx_lengths[dma_buffer_index] = 0U;
        dma_buffer_index = UART_DMA_TX_INDEX_INVALID;  // Mark DMA as idle
    }
}

/// @brief Start a DMA transfer when data is pending and DMA1 is idle.
/// @param None
/// @return None
static void UART_DMA_TX_StartNext(void)
{
    uint8_t dma_buffer;
    uint16_t transfer_count;

    /* Allow forward progress even when DMA completion IRQ is not firing. */
    UART_DMA_TX_ServiceDma();

    if (DMA_IsTransferInProgress(1U))
    {
        if (dma_buffer_index != UART_DMA_TX_INDEX_INVALID)
        {
            return;
        }

        /* Stale busy with no owner: clear and continue. */
        DMA_SelectChannel(1U);
        DMAnCON0bits.DGO = 0U;
        DMAnCON0bits.EN = 0U;
        DMAnCON0bits.SIRQEN = 1U;
        DMAnCON0bits.AIRQEN = 0U;
        DMAnCON0bits.EN = 1U;
    }

    if (tx_lengths[fill_buffer_index] != 0U)
    {
        /* The buffer currently being filled has data - hand it to DMA and
           swap fill_buffer_index to the other buffer using XOR toggle.
           XOR with 1 flips between index 0 and 1: 0^1=1, 1^1=0. */
        dma_buffer = fill_buffer_index;
        fill_buffer_index ^= 1U;  // Switch application fill target to the other buffer
    }
    else if (tx_lengths[fill_buffer_index ^ 1U] != 0U)
    {
        /* The fill buffer is empty but the other buffer still has data - use that. */
        dma_buffer = (uint8_t)(fill_buffer_index ^ 1U);
    }
    else
    {
        return;
    }

    dma_buffer_index = dma_buffer;
    transfer_count = tx_lengths[dma_buffer];

    DMA1_KickUartTx(tx_buffers[dma_buffer], transfer_count);
}

/// @brief Reset buffer/DMA state used by the TX engine.
/// @param None
/// @return None
void UART_DMA_TX_StateInitialize(void)
{
    DMA_SelectChannel(1U);
    DMAnCON0bits.DGO = 0U;
    DMAnCON0bits.EN = 1U;
    DMAnCON0bits.SIRQEN = 1U;
    DMAnCON0bits.AIRQEN = 0U;

    fill_buffer_index = 0U;
    dma_buffer_index = UART_DMA_TX_INDEX_INVALID;
    dma_stall_counter = 0U;
    tx_lengths[0] = 0U;
    tx_lengths[1] = 0U;
}

/// @brief putch() backend for printf(): enqueue bytes and trigger 
///        DMA transmission.  The function checks for buffer space 
///        and waits if necessary, while also servicing the DMA 
///        engine to ensure progress. If a line terminator is 
///        detected, the function will attempt to flush immediately, 
///        but will not wait for buffer space if the next byte(s) 
///        are already enqueued. The function also checks for buffer 
///        space and waits if necessary, while also servicing the DMA 
///        engine to ensure progress. If a line terminator is detected, 
///        the function will attempt to flush immediately, but will not 
///        wait for buffer space if the next byte(s) are already enqueued.
/// @param byte The character to be transmitted.    
/// @return None
void putch(char byte)
{
    uint8_t fill_buffer;
    uint16_t fill_length;
    bool flush_now;
    uint8_t gie_state;

    for (;;)
    {
        /* Save and disable global interrupts around the critical section.
           The DMA ISRs also modify fill_buffer_index and tx_lengths, so we must
           prevent them from running while we read/modify those shared variables.
           The saved state is restored at the end, preserving whatever interrupt
           enable state the caller had (e.g., the ISR itself may call putch). */
        gie_state = INTCON0bits.GIE;
        INTCON0bits.GIE = 0U;  // Disable interrupts - start of critical section

        UART_DMA_TX_ServiceDma();  // Check for DMA completion/errors before adding new data

        fill_buffer = fill_buffer_index;             // Snapshot current fill buffer index
        fill_length = tx_lengths[fill_buffer];       // How many bytes are already in that buffer

        if (fill_length < UART_BUFFER_SIZE)
        {
            tx_buffers[fill_buffer][fill_length] = byte;              // Append byte to fill buffer
            tx_lengths[fill_buffer] = (uint16_t)(fill_length + 1U);   // Update length

            /* Detect a line terminator (\r\n or \n\r pair) and flush the buffer
               immediately.  This ensures that complete lines appear on the terminal
               without waiting for the buffer to fill up. */
            flush_now = false;
            if (fill_length != 0U)
            {
                uint8_t previous = tx_buffers[fill_buffer][fill_length - 1U];
                if (((previous == '\r') && (byte == '\n')) ||
                    ((previous == '\n') && (byte == '\r')))
                {
                    flush_now = true;  // Line terminator detected - flush now
                }
            }

            if (flush_now)
            {
                UART_DMA_TX_StartNext();  // Immediately hand the line to DMA
            }

            /* Always call StartNext to keep the DMA engine moving forward even
               when we did not detect a line terminator.  If DMA is already busy
               this call returns quickly without disrupting the active transfer. */
            UART_DMA_TX_StartNext();

            INTCON0bits.GIE = gie_state;  // Restore interrupts - end of critical section
            break;  // Byte was successfully queued - exit the retry loop
        }

        /* Buffer full: try to start DMA to drain it, then loop back and retry.
           We restore interrupts briefly to allow the DMA ISR to service completions
           before checking again, avoiding a deadlock. */
        UART_DMA_TX_StartNext();
        INTCON0bits.GIE = gie_state;
    }
}

/// @brief Common DMA1 ISR handler for TX service/restart.
///        This function is called by all DMA1-related ISRs to handle completion and error conditions,
///        as well as to restart the next transfer if pending data is available. By centralizing
///        the DMA service logic in this function, we ensure consistent handling of all DMA events
///        related to UART transmission, while keeping the individual ISRs simple and focused on
///        delegating to this common handler. The function checks for completion and error flags,
///        clears them, and takes appropriate action such as aborting and recovering from errors,
///        or starting the next transfer if the current one has completed and there is pending data.
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

/// @brief DMA1 source-count ISR. This ISR is triggered when the source-count condition is 
/// met for DMA1, indicating that the specified number of bytes has been transferred from the 
/// source address. The ISR calls the common UART_DMA_TX_ISR handler to service the completion 
/// and potentially start the next transfer if there is pending data in the buffers. By handling 
/// this interrupt, we can ensure that the DMA engine continues to operate smoothly and that 
/// any necessary cleanup or state updates are performed after each block of data is transmitted 
/// via UART.
/// @param None
/// @return None
void __interrupt(irq(IRQ_DMA1SCNT), low_priority) DMA1_SourceCount_ISR(void)
{
    UART_DMA_TX_ISR();
}

/// @brief DMA1 destination-count ISR. This ISR is triggered when the destination-count 
/// condition is met for DMA1, indicating that the specified number of bytes has been 
/// transferred to the destination address. Similar to the source-count ISR, this function 
/// calls the common UART_DMA_TX_ISR handler to manage the completion of the transfer and 
/// to check if there is more data pending for transmission. Handling this interrupt allows 
/// us to maintain the flow of data through the DMA engine and ensures that any necessary 
/// actions are taken when a block of data has finished transmitting via UART.
/// @param None
/// @return None
void __interrupt(irq(IRQ_DMA1DCNT), low_priority) DMA1_DestCount_ISR(void)
{
    UART_DMA_TX_ISR();
}

/// @brief DMA1 overrun/other-error ISR.  This ISR is triggered when an overrun or other 
/// error condition occurs during a DMA1 transfer.
/// @param None
/// @return None
void __interrupt(irq(IRQ_DMA1OR), low_priority) DMA1_OverRun_ISR(void)
{
    UART_DMA_TX_ISR();
}

/// @brief DMA1 address-error ISR.  This ISR is triggered when an address-error condition 
/// occurs during a DMA1 transfer.
/// @param None
/// @return None
void __interrupt(irq(IRQ_DMA1A), low_priority) DMA1_A_ISR(void)
{
    UART_DMA_TX_ISR();
}
