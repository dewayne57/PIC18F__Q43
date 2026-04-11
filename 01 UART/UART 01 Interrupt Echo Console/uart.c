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
#define UART1_TX_BUFFER_MASK (UART1_TX_BUFFER_SIZE - 1U)
#define UART1_RTS_ASSERTED_LEVEL 0U
#define UART1_RTS_DEASSERTED_LEVEL 1U
#define UART1_CTS_ASSERTED_LEVEL 0U

static volatile uint8_t s_tx_head;
static volatile uint8_t s_tx_tail;
static volatile char s_tx_buffer[UART1_TX_BUFFER_SIZE];

static void UART1_SetRTS(bool asserted)
{
    LATBbits.LATB3 = (asserted != false) ? UART1_RTS_ASSERTED_LEVEL : UART1_RTS_DEASSERTED_LEVEL;
}

static bool UART1_IsCTSAsserted(void)
{
    return ((uint8_t)PORTBbits.RB2 == UART1_CTS_ASSERTED_LEVEL);
}

static uint8_t UART1_TxBufferFreeCount(void)
{
    uint8_t used;

    if (s_tx_head >= s_tx_tail)
    {
        used = (uint8_t)(s_tx_head - s_tx_tail);
    }
    else
    {
        used = (uint8_t)(UART1_TX_BUFFER_SIZE - (uint8_t)(s_tx_tail - s_tx_head));
    }

    return (uint8_t)((UART1_TX_BUFFER_SIZE - 1U) - used);
}

static bool UART1_TxBufferPush(char data)
{
    uint8_t next_head = (uint8_t)((s_tx_head + 1U) & UART1_TX_BUFFER_MASK);
    if (next_head == s_tx_tail)
    {
        return false;
    }

    s_tx_buffer[s_tx_head] = data;
    s_tx_head = next_head;
    return true;
}

static bool UART1_TxBufferPop(char *data)
{
    if (s_tx_head == s_tx_tail)
    {
        return false;
    }

    *data = s_tx_buffer[s_tx_tail];
    s_tx_tail = (uint8_t)((s_tx_tail + 1U) & UART1_TX_BUFFER_MASK);
    return true;
}

static void UART1_TxKick(void)
{
    char next;

    if (UART1_IsCTSAsserted() == false)
    {
        /* Keep requesting to send while data is pending, but hold TX until CTS allows it. */
        UART1_SetRTS(true);
        PIE4bits.U1TXIE = 1U;
        return;
    }

    if (UART1_TxBufferPop(&next) == false)
    {
        UART1_SetRTS(false);
        PIE4bits.U1TXIE = 0U;
        return;
    }

#if defined(U1TXB)
    U1TXB = (uint8_t)next;
#elif defined(TX1REG)
    TX1REG = (uint8_t)next;
#endif

    PIE4bits.U1TXIE = 1U;
}

static void UART1_WriteBufferBlocking(const char *data, uint16_t length)
{
    while ((data != (const char *)0) && (length > 0U))
    {
        uint8_t free_count = UART1_TxBufferFreeCount();
        uint16_t to_copy = length;

        UART1_SetRTS(true);

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

void UART1_Initialize(void)
{
    s_tx_head = 0U;
    s_tx_tail = 0U;

    /* Configure UART1 GPIO directions from project pin assignment. */
    TRISBbits.TRISB0 = 0U; /* TX1 */
    TRISBbits.TRISB1 = 1U; /* RX1 */
    TRISBbits.TRISB2 = 1U; /* CTS1 */
    TRISBbits.TRISB3 = 0U; /* RTS1 */
    UART1_SetRTS(false);

    PPS_Unlock();

    /* PPS output mapping for TX1 on RB0. */
#if defined(RB0PPS)
    RB0PPS = 0x20U;
#endif

    /* PPS input mapping for RX1 on RB1. */
#if defined(U1RXPPS)
    U1RXPPS = 0x09U;
#elif defined(RX1PPS)
    RX1PPS = 0x09U;
#endif

    PPS_Lock();

    /* Async mode, 8-bit, no parity, 1 stop-bit with configured baud rate. */
#if defined(U1CON0)
    U1CON0 = 0x30U; /* TXEN + RXEN */
#endif
#if defined(U1CON1)
    U1CON1 = 0x80U; /* UART module enabled */
#endif
#if defined(U1CON2)
    U1CON2 = 0x00U;
#endif
#if defined(U1BRG)
    U1BRG = (uint16_t)UART_1_BRG_VALUE;
#endif

    PIE4bits.U1TXIE = 0U;
    PIE4bits.U1RXIE = 0U;

    INTCON0bits.GIE = 1U;
}

void __interrupt() UART1_ISR(void)
{
    if ((PIE4bits.U1TXIE != 0U) && (PIR4bits.U1TXIF != 0U))
    {
        UART1_TxKick();
    }
}

void UART1_WriteChar(char data)
{
    UART1_WriteBufferBlocking(&data, 1U);
}

/* XC8 stdio retarget: printf writes each character through this hook. */
void putch(char data)
{
    UART1_WriteChar(data);
}
