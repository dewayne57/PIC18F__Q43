/* *****************************************************************************************
 *   File Name: uart.c
 *   Description: Module scaffold for UART 01 Interrupt Echo Console.
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
 *   functions.  The UART is configured for 9600 baud, no parity, 8 data bits, and 1 stop
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
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "pps.h"
#include "uart.h"

#define UART1_TX_BUFFER_SIZE 128U
#define UART1_TX_BUFFER_LIMIT (UART1_TX_BUFFER_SIZE - 1U)
#define UART1_RX_BUFFER_SIZE 128U
#define UART1_RX_BUFFER_LIMIT (UART1_RX_BUFFER_SIZE - 1U)
#define UART1_RTS_ASSERTED_LEVEL 0U
#define UART1_RTS_DEASSERTED_LEVEL 1U
#define UART1_CTS_ASSERTED_LEVEL 0U

static volatile uint8_t tx_head;
static volatile uint8_t tx_tail;
static volatile char tx_buffer[UART1_TX_BUFFER_SIZE];
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;
static volatile char rx_buffer[UART1_RX_BUFFER_SIZE];
static bool uart1_initialized = false;

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
    tx_head = 0U;
    tx_tail = 0U;
    rx_head = 0U;
    rx_tail = 0U;

    /* Configure UART1 GPIO directions from project pin assignment. */
    TRISBbits.TRISB0 = 0U; /* TX1 - Output*/
    TRISBbits.TRISB1 = 1U; /* RX1 - Input */
    TRISBbits.TRISB2 = 1U; /* CTS1 - Input */
    TRISBbits.TRISB3 = 0U; /* RTS1 - Output */

    /* Unlock PPS registers for configuration.   */
    PPS_Unlock();

    /* PPS output mapping for TX1 on RB0. */
    RB0PPS = 0x20U;   /* RB0 is TX1 output, so set to UART1 TX function */
    RB3PPS = 0x22U;   /* RB3 is RTS1 output, so set to UART1 RTS function */
    U1RXPPS = 0x09U;  /* RB1 is RX1 input, so set to UART1 RX function */
    U1CTSPPS = 0x0AU; /* RB2 is CTS1 input, so set to UART1 CTS function */

    /* Lock PPS registers after configuration. */
    PPS_Lock();

    /* Async mode, 8-bit, no parity, 1 stop-bit with configured baud rate and HW flow control. */
    U1CON0 = 0x30U; /* TXEN + RXEN */
    U1CON1 = 0x80U; /* UART module enabled */
    U1CON2 = 0x02U; /* CTS/RTS flow control enabled */

    U1BRG = (uint16_t)UART_1_BRG_VALUE;

    /* Disable UART1 transmit and receive interrupts for now. */
    U1ERRIR = 0x00U; /* Clear all error flags */
    U1ERRIEbits.U1TXMTIE = 1U; /* Enable TX empty interrupt */
    
    PIE4bits.U1TXIE = 0U;
    PIE4bits.U1RXIE = 1U;
    INTCON0bits.GIE = 1U;

    uart1_initialized = true;
}

/// @brief Returns the number of free bytes in the UART1 transmit buffer.
/// @param None
/// @return The number of free bytes in the transmit buffer.
static uint8_t UART1_TxBufferFreeCount(void)
{
    uint8_t used;

    if (tx_head >= tx_tail)
    {
        used = (uint8_t)(tx_head - tx_tail);
    }
    else
    {
        used = (uint8_t)(UART1_TX_BUFFER_SIZE - (uint8_t)(tx_tail - tx_head));
    }

    return (uint8_t)((UART1_TX_BUFFER_SIZE - 1U) - used);
}

/// @brief Pushes a character into the UART1 transmit buffer.
/// @param data The character to be pushed into the buffer.
/// @return true if the character was successfully pushed, false if the buffer is full.
static bool UART1_TxBufferPush(char data)
{
    uint8_t next_head = (uint8_t)((tx_head + 1U) & UART1_TX_BUFFER_LIMIT);
    if (next_head == tx_tail)
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
    uint8_t next_head = (uint8_t)((rx_head + 1U) & UART1_RX_BUFFER_LIMIT);
    if (next_head == rx_tail)
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

    U1TXB = (uint8_t)next;
    PIE4bits.U1TXIE = 1U;
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
            /* Software RX buffer is full, block further RX interrupts until data is consumed. */
            PIE4bits.U1RXIE = 0U;
        }
        else
        {
            (void)UART1_RxBufferPush((char)U1RXB);
        }
        PIR4bits.U1RXIF = 0U;
    }
}

/// @brief Handles the UART1 transmit interrupt. This function checks if the transmit interrupt is 
///        enabled and if the transmit interrupt flag is set. If so, it calls UART1_SendNext to
///        send the next character from the transmit buffer. If the buffer is empty, the transmit
///        interrupt will be disabled by UART1_SendNext until new data is added to the buffer.
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
        PIR4bits.U1TXIF = 0U;
    }
}

/// @brief Writes a buffer of data to the UART1 transmit buffer in a blocking manner.
/// @param data Pointer to the buffer containing the data to be transmitted.
/// @param length The number of bytes to be transmitted.
/// @return None
static void UART1_WriteBufferBlocking(const char *data, uint16_t length)
{
    while ((data != (const char *)0) && (length > 0U))
    {
        uint8_t free_count = UART1_TxBufferFreeCount();
        uint16_t to_copy = length;

        if (to_copy > free_count)
        {
            to_copy = (uint16_t)free_count;
        }

        while (to_copy > 0U)
        {
            (void)UART1_TxBufferPush(*data);
            data++;
            length--;
            to_copy--;
        }

        PIE4bits.U1TXIE = 1U;

        if (length > 0U)
        {
            while (UART1_TxBufferFreeCount() == 0U)
            {
                /* Wait until ISR drains at least one byte. */
            }
        }
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
/// @note This function uses the UART1_WriteBufferBlocking function to ensure that the character is added
///       to the transmit buffer and transmitted over UART1. Any calls to printf in the application will 
///       ultimately call this function for each character, enabling seamless integration of UART output 
///       with standard C library functions.  Any output sent to printf will be discarded if UART1 is not 
///       initialized, so it is important to call UART1_Initialize before using printf for UART output.
void putch(char data)
{
    if (uart1_initialized)
    {
        UART1_WriteBufferBlocking(&data, 1U);
    }
}
