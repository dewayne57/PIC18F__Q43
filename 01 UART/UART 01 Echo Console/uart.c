/* *****************************************************************************************
 *   File Name: uart.c
 *   Description: Module UART 01 Interrupt Echo Console.
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
 *
 *   This module provides the implementation for UART 1 initialization and basic transmit
 *   functions.  The UART is configured for 57,600 baud, no parity, 8 data bits, and 1 stop
 *   bit. The transmit function is interrupt-driven using a ring buffer to hold outgoing
 *   data. The receive function is also interrupt-driven, allowing for efficient handling
 *   of incoming data.  The use of a ring buffer (aka, circular queue) allows for non-
 *   blocking transmission of data, enabling the main application to continue running while
 *   data is being sent over the UART.  This allows for a responsive application that can
 *   handle other tasks while still providing UART communication capabilities.
 *
 *   The code checks that the ring buffer has room before adding data to it, and the
 *   transmit interrupt is enabled when data is added.  If there is not enough room in
 *   the buffer to transmit the data, what will fit is moved into the buffer and the caller
 *   will block until the ISR has made room for the remaining data.  This allows for
 *   efficient use of the buffer while still ensuring that all data is transmitted without
 *   loss.
 *
 *   It is important to note that the transmit buffer empty interrupt is only generated
 *   when the UART is enabled AND the transmit buffer is empty, so the initial transmission
 *   must be triggered by adding data to the buffer and enabling the transmit interrupt. When
 *   all transmission is complete and the buffer is empty, the transmit interrupt will be
 *   disabled until new data is added to the buffer, at which point the interrupt will be
 *   enabled again to start the transmission process. This ensures that the ISR is only
 *   called when there is data to be sent, allowing for efficient use of CPU resources
 *   while still providing responsive UART communication.
 *
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "pps.h"
#include "uart.h"

#define UART1_TX_BUFFER_SIZE 128U            // Number of bytes in the TX ring buffer (must be a power of 2)
#define UART1_TX_BUFFER_LIMIT (UART1_TX_BUFFER_SIZE - 1U) // Bitmask used for fast circular wrap: (index + 1) & LIMIT
#define UART1_RX_BUFFER_SIZE 128U            // Number of bytes in the RX ring buffer (must be a power of 2)
#define UART1_RX_BUFFER_LIMIT (UART1_RX_BUFFER_SIZE - 1U) // Bitmask used for fast circular wrap
#define UART1_RTS_ASSERTED_LEVEL 0U          // RTS is active-low: drive pin LOW to signal "ready to receive"
#define UART1_RTS_DEASSERTED_LEVEL 1U        // Drive pin HIGH to signal "stop sending"
#define UART1_CTS_ASSERTED_LEVEL 0U          // CTS input is active-low: LOW means remote side is ready

/* Ring buffer state for transmit and receive.  The buffer is treated as a circular
   queue: head points to the next write slot, tail points to the next read slot.
   When head == tail the buffer is empty; it is full when advancing head would
   make it equal to tail (one slot is always kept free to distinguish full/empty). */
static volatile uint8_t tx_head;             // Next position to write into TX buffer
static volatile uint8_t tx_tail;             // Next position to read from TX buffer
static volatile char tx_buffer[UART1_TX_BUFFER_SIZE];
static volatile uint8_t rx_head;             // Next position to write into RX buffer
static volatile uint8_t rx_tail;             // Next position to read from RX buffer
static volatile char rx_buffer[UART1_RX_BUFFER_SIZE];
static bool uart1_initialized = false;       // Guard flag - prevents using UART before init

/// @brief Apply configured UART frame format to MODE and stop-bit settings.
/// @param None
/// @return None
static void UART1_ApplyFrameFormat(void)
{
    /* U1CON0.MODE selects the framing format:
       0x0 = 8-bit data, no parity (most common setting)
       0x2 = 9-bit data, no parity  (used for multi-drop addressing)
       0x8 = 8-bit data, odd parity bit appended
       0x9 = 8-bit data, even parity bit appended
       Default to 8-bit no parity, then override as needed. */
    U1CON0bits.MODE = 0x0;

    if (UART_1_DATA_BITS == 9)
    {
        U1CON0bits.MODE = 0x2;  // 9-bit mode - 9th bit used for parity or multi-drop addressing
    }
    else if (UART_1_PARITY == UART_PARITY_ODD)
    {
        U1CON0bits.MODE = 0x8;  // Hardware inserts/checks odd parity bit
    }
    else if (UART_1_PARITY == UART_PARITY_EVEN)
    {
        U1CON0bits.MODE = 0x9;  // Hardware inserts/checks even parity bit
    }

    /* U1CON2.STP selects the number of stop bits transmitted after each byte:
       UART_STOP_BITS_1   = 0 -> 1 stop bit   (standard)
       UART_STOP_BITS_1_5 = 1 -> 1.5 stop bits (used with 5-bit data on some terminals)
       UART_STOP_BITS_2   = 2 -> 2 stop bits   (older or noisy-line protocols) */
    switch (UART_1_STOP_BITS)
    {
    case UART_STOP_BITS_1_5:
    case UART_STOP_BITS_2:
        U1CON2bits.STP = UART_1_STOP_BITS;
        break;

    case UART_STOP_BITS_1:
    default:
        U1CON2bits.STP = UART_STOP_BITS_1;
        break;
    }
}

/// @brief Initializes UART1 for asynchronous communication with the specified settings.
///        Configures GPIO pins for UART functionality, sets up the baud rate, and enables the
///        UART module and its interrupts. The function also initializes the transmit buffer
///        indices. This setup allows for efficient UART communication using interrupts and
///        a ring buffer for outgoing data, enabling non-blocking transmission.
/// @param None
/// @return None
/// @note This function should be called before any other UART1 functions to ensure proper
///       initialization. The actual baud rate is determined by the value of UART_1_BRG_VALUE,
///       which should be defined according to the desired baud rate and the system clock frequency.
void UART1_Initialize(void)
{
    tx_head = 0;
    tx_tail = 0;
    rx_head = 0;
    rx_tail = 0;

    /* Configure UART1 GPIO directions from project pin assignment. */
    TRISBbits.TRISB0 = 0; /* TX1 - Output*/
    TRISBbits.TRISB1 = 1; /* RX1 - Input */

    /* Unlock PPS registers for configuration.   */
    PPS_Unlock();

    RB0PPS = 0x20;  /* RB0 is TX1 output, so set to UART1 TX function */
    U1RXPPS = 0x09; /* RB1 is RX1 input, so set to UART1 RX function */

    /* Lock PPS registers after configuration. */
    PPS_Lock();

    /* Async mode from config, no HW flow control (2-wire UART). */
    U1CON2 = 0x00;
    U1CON2bits.RUNOVF = 1; /* RUNOVF=1: receiver keeps running and sets error flag on overflow
                              instead of halting - prevents incoming data from being silently lost */
    U1CON2bits.FLO = 0;    /* FLO: flow control mode - 0=none, 1=XON/XOFF software, 2=RTS/CTS hardware */
    /* Load the baud rate register.  The value was pre-calculated in config.h from:
       BRG = (Fosc / (16 * BaudRate)) - 1   (for standard speed)
       Higher BRG values = slower baud rate; see datasheet section on UART baud rate generation. */
    U1BRG = (uint16_t)UART_1_BRG_VALUE;

    U1ERRIR = 0x00;          /* Clear all UART error flags before enabling */
    U1ERRIEbits.U1TXMTIE = 1; /* Enable the TX shift-register-empty interrupt used by UART1_SendNext */

    /* TX interrupt is left DISABLED here intentionally.  The PIC18 UART only fires the
       TX interrupt when the transmit buffer is empty AND the interrupt is enabled.  If we
       enabled it now (with an empty buffer) the CPU would immediately enter the ISR and
       spin.  Instead, the interrupt is enabled by putch()/UART1_WriteBufferBlocking() only
       after the first byte is placed in the ring buffer, which "primes the pump". */
    PIE4bits.U1TXIE = 0;    /* TX interrupt OFF - enabled later when data is ready to send */
    PIE4bits.U1RXIE = 0;    /* RX interrupt OFF - enabled after UART module is fully running */
#ifdef UART1_VECTORED_INTERRUPTS
    /* If using vectored interrupts, enable both low and high priority interrupts. */
    INTCON0bits.GIEL = 1; /* Enable low priority interrupts. */
    INTCON0bits.GIEH = 1; /* Enable high priority interrupts. */
#else
    /* If not using vectored interrupts, enable global interrupts. */
    INTCON0bits.GIE = 1; /* Enable global interrupts. */
#endif

    U1CON0 = 0x00;               // Clear all CON0 bits before applying frame settings
    UART1_ApplyFrameFormat();   // Apply data bits, parity, and stop bits from config.h
    U1CON0bits.RXEN = 1;        // Enable the receiver
    U1CON0bits.TXEN = 1;        // Enable the transmitter
    U1CON1 = 0x00;
    U1CON1bits.ON = 1;          // Turn the UART module on - must be done AFTER configuring CON0/CON2
    __delay_ms(10);             // Short delay to allow the UART line to settle before first use
    char dummy = U1RXB;         // Read (and discard) any byte that appeared during power-up glitch
    PIE4bits.U1RXIE = 1;        // Now safe to enable RX interrupts - UART is fully running

    uart1_initialized = true;
}

/// @brief Returns the number of free bytes in the UART1 transmit buffer.
/// @param None
/// @return The number of free bytes in the transmit buffer.
static uint8_t UART1_TxBufferFreeCount(void)
{
    uint8_t used;

    /* Calculate the number of bytes currently used in the circular buffer.
       When head >= tail the data runs from tail up to head (no wrap-around).
       When head < tail the data has wrapped: used bytes = SIZE - (tail - head). */
    if (tx_head >= tx_tail)
    {
        used = (uint8_t)(tx_head - tx_tail);
    }
    else
    {
        used = (uint8_t)(UART1_TX_BUFFER_SIZE - (uint8_t)(tx_tail - tx_head));
    }

    /* One slot is reserved to distinguish "empty" (head==tail) from "full",
       so the maximum usable capacity is SIZE - 1. */
    return (uint8_t)((UART1_TX_BUFFER_SIZE - 1U) - used);
}

/// @brief Pushes a character into the UART1 transmit buffer.
/// @param data The character to be pushed into the buffer.
/// @return true if the character was successfully pushed, false if the buffer is full.
static bool UART1_TxBufferPush(char data)
{
    /* Advance head by 1, wrapping around using a bitwise AND with the buffer limit
       (works because the buffer size is a power of 2, so LIMIT = SIZE - 1 acts as a modulo mask). */
    uint8_t next_head = (uint8_t)((tx_head + 1U) & UART1_TX_BUFFER_LIMIT);
    if (next_head == tx_tail)   // If the next head position would reach the tail, the buffer is full
    {
        return false;
    }

    tx_buffer[tx_head] = data;  // Store the character at the current head position
    tx_head = next_head;        // Advance head to the next slot
    return true;
}

/// @brief Pops a character from the UART1 transmit buffer.
/// @param data Pointer to the variable where the popped character will be stored.
/// @return true if a character was successfully popped, false if the buffer is empty.
static bool UART1_TxBufferPop(char *data)
{
    if (tx_head == tx_tail)
    {
        return false;
    }

    *data = tx_buffer[tx_tail];
    tx_tail = (uint8_t)((tx_tail + 1U) & UART1_TX_BUFFER_LIMIT);
    return true;
}

/// @brief Returns the number of free bytes in the UART1 receive buffer.
/// @param None
/// @return The number of free bytes in the receive buffer.
static uint8_t UART1_RxBufferFreeCount(void)
{
    uint8_t used;

    if (rx_head >= rx_tail)
    {
        used = (uint8_t)(rx_head - rx_tail);
    }
    else
    {
        used = (uint8_t)(UART1_RX_BUFFER_SIZE - (uint8_t)(rx_tail - rx_head));
    }

    return (uint8_t)((UART1_RX_BUFFER_SIZE - 1U) - used);
}

/// @brief Pushes a character into the UART1 receive buffer.
/// @param data The character to be pushed into the buffer.
/// @return true if the character was successfully pushed, false if the buffer is full.
static bool UART1_RxBufferPush(char data)
{
    /* Discard null bytes (0x00) - these commonly appear as line glitches during power-up
       or when the UART RX line is left floating, and can be safely ignored for a text console. */
    if (data == 0x00)
    {
        return true;
    }

    uint8_t next_head = (uint8_t)((rx_head + 1U) & UART1_RX_BUFFER_LIMIT); // Circular advance
    if (next_head == rx_tail)   // Buffer full - discard incoming byte rather than overwrite unread data
    {
        return false;
    }

    rx_buffer[rx_head] = data;
    rx_head = next_head;
    return true;
}

/// @brief Pops a character from the UART1 receive buffer.
/// @param data Pointer to the variable where the popped character will be stored.
/// @return true if a character was successfully popped, false if the buffer is empty.
static bool UART1_RxBufferPop(char *data)
{
    if (rx_head == rx_tail)
    {
        return false;
    }

    *data = rx_buffer[rx_tail];
    rx_tail = (uint8_t)((rx_tail + 1U) & UART1_RX_BUFFER_LIMIT);
    return true;
}
/// @brief Sends the next character from the UART1 transmit buffer. If the buffer is empty, the transmit
///        interrupt is disabled. Otherwise, the next character is loaded into the UART transmit register
///        and the transmit interrupt is enabled to continue sending remaining characters in the buffer.
///        This function is typically called from the UART1 transmit interrupt service routine to handle
///        ongoing transmission of data from the buffer.
///
/// @param  None
/// @return None
static void UART1_SendNext(void)
{
    char next;

    if (!UART1_TxBufferPop(&next))
    {
        PIE4bits.U1TXIE = 0U;
        return;
    }

    U1TXB = (uint8_t)next; /* then write the character to the transmit buffer */
    PIE4bits.U1TXIE = 1U;  /* Enable transmit interrupt to send next character when ready */
}

/// @brief Handles the UART1 receive interrupt. This function checks if the receive interrupt is
///        enabled and if the receive interrupt flag is set.
/// @param  None
/// @return None
/// @note This function can be generated as a low priority interrupt service routine if
///       UART1_VECTORED_INTERRUPTS is set to 1.  If not set, this function should be
///       called from the main ISR to handle UART1 interrupts.
#if UART1_VECTORED_INTERRUPTS
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
#else
void UART1_RX_ISR(void)
#endif
{
    if ((PIE4bits.U1RXIE != 0U) && (PIR4bits.U1RXIF != 0U))
    {
        if (UART1_RxBufferFreeCount() == 0U)
        {
            /* Software RX buffer is full; drain the hardware FIFO byte so RXIF clears,
               then block further RX interrupts until the application consumes data. */
            (void)U1RXB;
            PIE4bits.U1RXIE = 0U;
        }
        else
        {
            (void)UART1_RxBufferPush((char)U1RXB);
        }
    }
}

/// @brief Handles the UART1 transmit interrupt. This function checks if the transmit interrupt is
///        enabled and if the transmit interrupt flag is set. If so, it calls UART1_SendNext to
///        send the next character from the transmit buffer. If the buffer is empty, the transmit
///        interrupt will be disabled by UART1_SendNext until new data is added to the buffer.
///        The Q43 datasheet indicates that this interrupt is generated only when the UART is
///        enabled and the transmit buffer is empty, so this function will be called whenever the
///        UART is ready to send the next character, allowing for efficient transmission of data from
///        the buffer without blocking the main application.  However, the initial send must be
///        triggered by adding data to the buffer and enabling the transmit interrupt, which is handled
///        by the UART1_WriteBufferBlocking function. This means we need to toggle the interrupt
///        enable bit to start the transmission process when new data is added to the buffer, and the
///        ISR will take care of sending the data and disabling the interrupt when the buffer is empty.
///
/// @param  None
/// @return None
#if UART1_VECTORED_INTERRUPTS
void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
#else
void UART1_TX_ISR(void)
#endif
{
    if ((PIE4bits.U1TXIE != 0U) && (PIR4bits.U1TXIF != 0U))
    {
        UART1_SendNext();
    }
}

/// @brief Reads one character from the UART1 receive buffer.
/// @param data Pointer to destination for the received character.
/// @return true if one character was read, false if no data is available.
bool UART1_ReadChar(char *data)
{
    if ((data == (char *)0) || (!uart1_initialized))
    {
        return false;
    }

    if (!UART1_RxBufferPop(data))
    {
        return false;
    }

    /* Resume RX interrupts when at least one slot becomes free. */
    PIE4bits.U1RXIE = 1U;
    return true;
}

/// @brief Returns the number of bytes currently buffered in the UART1 receive queue.
/// @param None
/// @return Number of buffered receive bytes.
uint8_t UART1_RxAvailable(void)
{
    if (rx_head >= rx_tail)
    {
        return (uint8_t)(rx_head - rx_tail);
    }

    return (uint8_t)(UART1_RX_BUFFER_SIZE - (uint8_t)(rx_tail - rx_head));
}

/// @brief Retarget function for printf to write characters to UART1. This function is called by the
///        XC8 standard library when printf is used, allowing for formatted output to be sent over UART1.
/// @param data The character to be transmitted.
/// @return None
void putch(char data)
{
    if (uart1_initialized)
    {
        if (UART1_TxBufferFreeCount() < 1)
        {
            /* Ring buffer is full - block here until the TX ISR has consumed at least
               one byte and made room.  The ISR runs in the background and will free
               space without any action needed from this loop. */
            while (UART1_TxBufferFreeCount() == 0U)
            {
            }
        }

        (void)UART1_TxBufferPush(data);   // Place the character into the ring buffer

        /* "Prime the pump": the TX interrupt only fires when the hardware FIFO is empty
           AND the interrupt enable bit (U1TXIE) is set.  If the FIFO has been empty while
           no interrupt was enabled, we manually send the first byte here to get the ISR
           chain started.  Once started, the ISR calls UART1_SendNext() automatically
           until the ring buffer drains, then disables itself to stop unnecessary interrupts. */
        if (U1FIFObits.TXBE == 1 && PIE4bits.U1TXIE == 0U)
        {
            /* TX hardware FIFO is empty and interrupts are off - kick off the first send. */
            UART1_SendNext();
        }
    }
}
