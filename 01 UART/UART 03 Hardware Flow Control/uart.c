/* *****************************************************************************************
 *   File Name: uart.c
 *   Description: UART implementation for the demonstration project. 
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
#define UART1_TX_BUFFER_LIMIT (UART1_TX_BUFFER_SIZE - 1U) // Bitmask used for fast circular wrap
#define UART1_RX_BUFFER_SIZE 128U            // Number of bytes in the RX ring buffer (must be a power of 2)
#define UART1_RX_BUFFER_LIMIT (UART1_RX_BUFFER_SIZE - 1U) // Bitmask used for fast circular wrap
/* RTS (Request To Send) - driven by this device to tell the remote it is ready to receive.
   Active-low: assert (0) = "I can accept data"; deassert (1) = "Stop sending, I'm busy". */
#define UART1_RTS_ASSERTED_LEVEL 0U
#define UART1_RTS_DEASSERTED_LEVEL 1U
/* CTS (Clear To Send) - driven by the remote device; read by this device before transmitting.
   Active-low: 0 = remote is ready to receive; 1 = remote is busy, pause transmission. */
#define UART1_CTS_ASSERTED_LEVEL 0U

/* Ring buffer state.  head = next write position, tail = next read position.
   head == tail means empty; advancing head to equal tail means full (one slot reserved). */
static volatile uint8_t tx_head;             // TX buffer write pointer
static volatile uint8_t tx_tail;             // TX buffer read pointer
static volatile char tx_buffer[UART1_TX_BUFFER_SIZE];
static volatile uint8_t rx_head;             // RX buffer write pointer
static volatile uint8_t rx_tail;             // RX buffer read pointer
static volatile char rx_buffer[UART1_RX_BUFFER_SIZE];
static bool uart1_initialized = false;       // Set true after UART1_Initialize() completes

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
    TRISBbits.TRISB0 = 0; /* RB0: TX1 output - UART drives this line */
    TRISBbits.TRISB1 = 1; /* RB1: RX1 input  - remote device drives this line */
    TRISBbits.TRISB2 = 0; /* RB2: RTS1 output - this device signals ready-to-receive */
    TRISBbits.TRISB3 = 1; /* RB3: CTS1 input  - remote device signals ready-to-receive */

    /* Unlock PPS registers for configuration.   */
    PPS_Unlock();

    /* PPS (Peripheral Pin Select) maps peripheral functions to physical pins:
       RB0PPS = 0x20 assigns UART1 TX output to pin RB0.
       U1RXPPS = 0x09 connects UART1 RX input from pin RB1 (Port B = 1, pin = 1 -> 0x09).
       RB2PPS = 0x22 assigns UART1 RTS output to pin RB2.
       U1CTSPPS = 0x0B connects UART1 CTS input from pin RB3 (Port B = 1, pin = 3 -> 0x0B).
       These constants come from the PIC18F47Q43 datasheet PPS output/input selection tables. */
    RB0PPS = 0x20;   /* RB0 -> UART1 TX output */
    U1RXPPS = 0x09;  /* UART1 RX input <- RB1 */
    RB2PPS = 0x22;   /* RB2 -> UART1 RTS output */
    U1CTSPPS = 0x0B; /* UART1 CTS input <- RB3 */

    /* Lock PPS registers after configuration. */
    PPS_Lock();

    /* Async mode from config with HW RTS/CTS flow control. */
    U1CON2 = 0x00;
    U1CON2bits.RUNOVF = 1; /* RUNOVF=1: receiver continues on overflow and sets error flag,
                              rather than halting, so incoming bytes are not silently lost */
    /* FLO selects UART1 flow control mode:
       0 = none (simple 2-wire TX/RX)
       1 = software XON/XOFF (UART 04 uses this)
       2 = hardware RTS/CTS - UART hardware drives RTS and monitors CTS automatically */
    U1CON2bits.FLO = 2;    /* Hardware RTS/CTS: UART controls RTS/CTS pins automatically */
    U1BRG = (uint16_t)UART_1_BRG_VALUE; /* Load baud rate divisor (see config.h for formula) */

    U1ERRIR = 0x00;          /* Clear all UART error flags before enabling */
    U1ERRIEbits.U1TXMTIE = 1; /* Enable TX shift-register-empty interrupt (used by UART1_SendNext) */

    /* TX interrupt starts disabled - it is enabled by putch() only when data is ready.
       Enabling it now with an empty buffer would cause an immediate spurious ISR. */
    PIE4bits.U1TXIE = 0;    /* TX interrupt off until data is queued */
    PIE4bits.U1RXIE = 0;    /* RX interrupt off until UART is fully started */
   
    #ifdef UART1_VECTORED_INTERRUPTS
    /* If using vectored interrupts, enable both low and high priority interrupts. */
    INTCON0bits.GIEL = 1; /* Enable low priority interrupts. */
    INTCON0bits.GIEH = 1; /* Enable high priority interrupts. */
    #else
    /* If not using vectored interrupts, enable global interrupts. */
    INTCON0bits.GIE = 1;    /* Enable global interrupts. */
    #endif

    // Set up data bits, parity, and stop bits based on configuration macros.
    U1CON0 = 0x00;
    /* U1CON0.MODE selects the UART framing format:
       0x0 = 8-bit, no parity (most common - used here by default)
       0x2 = 9-bit data (used for multi-drop addressing schemes)
       0x8 = 8-bit with odd parity (hardware automatically inserts/checks parity)
       0x9 = 8-bit with even parity */
    U1CON0bits.MODE = 0x0;  // Default: 8-bit, no parity

    if (UART_1_DATA_BITS == 9)
    {
        U1CON0bits.MODE = 0x2;  // 9-bit mode
    }
    else if (UART_1_PARITY == UART_PARITY_ODD)
    {
        U1CON0bits.MODE = 0x8;  // Odd parity
    }
    else if (UART_1_PARITY == UART_PARITY_EVEN)
    {
        U1CON0bits.MODE = 0x9;  // Even parity
    }
    /* U1CON2.STP selects stop bit count:
       UART_STOP_BITS_1   = 0 -> 1 stop bit  (standard for most applications)
       UART_STOP_BITS_1_5 = 1 -> 1.5 stop bits
       UART_STOP_BITS_2   = 2 -> 2 stop bits (used on noisy lines or older protocols) */
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

    // Now enable the UART module after all configurations are set.
    U1CON0bits.RXEN = 1;    // Enable receiver
    U1CON0bits.TXEN = 1;    // Enable transmitter
    U1CON1 = 0x00;
    U1CON1bits.ON = 1;      // Turn UART module on - must happen AFTER CON0/CON2 are configured
    __delay_ms(10);         // Brief settling delay before the first byte
    char dummy = U1RXB;     // Discard any byte that appeared during startup glitch
    PIE4bits.U1RXIE = 1;    // Enable RX interrupt - UART is now ready to receive
    
    uart1_initialized = true;
}

/// @brief Returns the number of free bytes in the UART1 transmit buffer.
/// @param None
/// @return The number of free bytes in the transmit buffer.
static uint8_t UART1_TxBufferFreeCount(void)
{
    uint8_t used;

    /* When head >= tail, data occupies bytes from tail up to head (no wrap).
       When head < tail, the data has wrapped around the end of the array. */
    if (tx_head >= tx_tail)
    {
        used = (uint8_t)(tx_head - tx_tail);
    }
    else
    {
        used = (uint8_t)(UART1_TX_BUFFER_SIZE - (uint8_t)(tx_tail - tx_head));
    }

    /* One slot is always kept empty to tell apart "empty" (head==tail) from "full". */
    return (uint8_t)((UART1_TX_BUFFER_SIZE - 1U) - used);
}

/// @brief Pushes a character into the UART1 transmit buffer.
/// @param data The character to be pushed into the buffer.
/// @return true if the character was successfully pushed, false if the buffer is full.
static bool UART1_TxBufferPush(char data)
{
    /* Advance head by 1 using a bitmask wrap (works because SIZE is a power of 2). */
    uint8_t next_head = (uint8_t)((tx_head + 1U) & UART1_TX_BUFFER_LIMIT);
    if (next_head == tx_tail)   // Buffer would be full - reject this byte
    {
        return false;
    }

    tx_buffer[tx_head] = data;
    tx_head = next_head;
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
    /* Discard null bytes - commonly seen during power-up or when RX line is undriven. */
    if (data == 0x00) { 
        return true;
    }
    
    uint8_t next_head = (uint8_t)((rx_head + 1U) & UART1_RX_BUFFER_LIMIT); // Circular advance
    if (next_head == rx_tail)   // Buffer full - drop byte to prevent overwriting unread data
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
    PIE4bits.U1TXIE = 1U; /* Enable transmit interrupt to send next character when ready */
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
            /* Ring buffer full - block until TX ISR frees at least one slot. */
            while (UART1_TxBufferFreeCount() == 0U)
            {
            }
        }

        (void)UART1_TxBufferPush(data);   // Queue character in ring buffer

        /* Prime the pump if the hardware FIFO is idle and the TX interrupt is off.
           Without this, the first byte after a quiet period would never be sent
           because the TX interrupt won't fire until the FIFO drains - which it
           can't do if it has never been primed. */
        if (U1FIFObits.TXBE == 1 && PIE4bits.U1TXIE == 0U)
        {
            UART1_SendNext();  // Kick off the TX interrupt chain
        }
    }
}
