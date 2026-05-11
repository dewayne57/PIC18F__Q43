/* *****************************************************************************************
 *   File Name: uartlib.h
 *   Description: Interrupt-driven multi-UART ring-buffer library for PIC18F Q43/Q84 devices.
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
 *   This library supports UART1 through UART5.  The application owns all configuration,
 *   ring-buffer storage, and per-UART state inside a uart_handle_t structure.  That handle
 *   is passed into every API call so one build can manage multiple UART peripherals at once.
 *
 *   Core usage model
 *   ----------------
 *   1. Create one uart_handle_t per UART peripheral you plan to use.
 *   2. Fill in the port number, framing configuration, and pointers to application-owned
 *      TX/RX buffers.
 *   3. Configure TRIS and PPS in the application.
 *   4. Call UART_Open(&handle).
 *   5. Route interrupts to UART_HandleRxInterrupt(&handle) and UART_HandleTxInterrupt(&handle),
 *      from application-owned ISR functions.
 *
 * ***************************************************************************************** */

#ifndef UARTLIB_H
#define UARTLIB_H

#include <stdbool.h>
#include <stdint.h>

/** UART peripheral selector used in uart_handle_t.port */
typedef enum
{
    UART_PORT_1 = 1,
    UART_PORT_2 = 2,
    UART_PORT_3 = 3,
    UART_PORT_4 = 4,
    UART_PORT_5 = 5
} uart_port_t;

/** Parity selection used in uart_handle_t.parity */
typedef enum
{
    UART_PARITY_NONE = 0,
    UART_PARITY_ODD  = 1,
    UART_PARITY_EVEN = 2
} uart_parity_t;

/** Stop-bit count used in uart_handle_t.stop_bits */
typedef enum
{
    UART_STOP_BITS_1   = 0,
    UART_STOP_BITS_1_5 = 1,
    UART_STOP_BITS_2   = 2
} uart_stop_bits_t;

/** Flow-control mode used in uart_handle_t.flow_control */
typedef enum
{
    UART_FLOW_NONE     = 0,
    UART_FLOW_XON_XOFF = 1,
    UART_FLOW_RTS_CTS  = 2
} uart_flow_t;

/**
 * @brief Application-owned UART instance definition.
 *
 * The application creates one of these structures for each UART peripheral it uses,
 * fills in the configuration fields, assigns buffer storage, and then passes the
 * structure to UART_Open().
 *
 * Buffer rules:
 * - tx_buffer and rx_buffer must point to valid arrays owned by the application.
 * - tx_buffer_size and rx_buffer_size must each be powers of 2.
 * - One slot in each ring buffer is always reserved so the library can distinguish
 *   "full" from "empty" without an extra flag.
 *
 * Framing rules:
 * - data_bits must be 8 or 9.
 * - flow_control values other than UART_FLOW_NONE are supported only on UART1,
 *   which is the full-featured UART on Q84 devices.
 * - high_speed_baud controls BRGS mode (false = standard, true = high speed).
 *
 * Baud rate formula:
 * - Standard speed (high_speed_baud=false): brg_value = (Fosc / (16 * BaudRate)) - 1
 * - High speed     (high_speed_baud=true):  brg_value = (Fosc /  (4 * BaudRate)) - 1
 */
typedef struct
{
    uint32_t          fosc;             /**< Peripheral clock frequency in Hz, used to calculate baud rate divisors */  
    uint32_t          baud_rate;        /**< Baud Rate Desired, such as 19200 */
    uint16_t          tx_buffer_size;   /**< Transmit buffer size, must be a power of 2 */
    uint16_t          rx_buffer_size;   /**< Receive buffer size, must be a power of 2 */
    uart_port_t       port;             /**< Which UART peripheral this handle controls */
    bool              high_speed_baud;  /**< Baud rate mode select: false=standard, true=high speed */
    uint8_t           data_bits;        /**< 8 or 9 data bits */
    uart_parity_t     parity;           /**< Parity mode */
    uart_stop_bits_t  stop_bits;        /**< Number of stop bits */
    uart_flow_t       flow_control;     /**< Flow control mode */
    char             *tx_buffer;        /**< Application-owned transmit ring buffer */
    char             *rx_buffer;        /**< Application-owned receive ring buffer */

    /* The user must not set, only initially clear, the following fields. */
    bool              initialized;      /**< Open/closed state: true=open, false=closed */
    volatile uint16_t tx_head;          /**< Next write slot in the transmit buffer */
    volatile uint16_t tx_tail;          /**< Next read slot in the transmit buffer */
    volatile uint16_t rx_head;          /**< Next write slot in the receive buffer */
    volatile uint16_t rx_tail;          /**< Next read slot in the receive buffer */
} uart_handle_t;

/**
 * @brief Open one UART instance described by an application-owned handle.
 *
 * The caller must configure TRIS and PPS before calling this function.  Global interrupt
 * enables are not modified here; only the selected UART interrupt enable bits are touched.
 *
 * If the handle is already open, this function does nothing and returns true.
 * To apply new configuration values, the application must call UART_Close(), update the
 * handle fields, then call UART_Open().
 *
 * On success, uart->initialized is set true to indicate the handle is open.
 *
 * @param uart  Pointer to a fully configured uart_handle_t structure.
 * @return true if the handle was valid and the peripheral was opened, false otherwise.
 */
bool UART_Open(uart_handle_t *uart);

/**
 * @brief Close one UART instance so it can be safely reconfigured.
 *
 * Disables UART interrupts and turns off the selected UART module.  This call also detaches
 * clears printf routing if this handle was the active printf target.
 *
 * If the handle is already closed, this function does nothing.
 *
 * After UART_Close(), uart->initialized is false (closed).
 *
 * @param uart  UART instance to close. NULL is ignored.
 */
void UART_Close(uart_handle_t *uart);

/**
 * @brief Queue one character for transmission on the selected UART.
 *
 * This function blocks while the TX ring buffer is full, matching the existing putch()
 * behavior used by printf().
 *
 * @param uart  UART instance to transmit on.
 * @param data  Character to queue.
 * @return true if the byte was queued, false if uart was NULL or closed.
 */
bool UART_WriteChar(uart_handle_t *uart, char data);

/**
 * @brief Read one character from the selected UART receive buffer.
 *
 * @param uart  UART instance to read from.
 * @param data  Destination for the received character.
 * @return true if one byte was read, false if no data is available, uart is closed, or arguments are invalid.
 */
bool UART_ReadChar(uart_handle_t *uart, char *data);

/**
 * @brief Return the number of bytes currently buffered in the selected UART receive queue.
 *
 * @param uart  UART instance to query.
 * @return Number of buffered bytes, or 0 if uart is NULL or closed.
 */
uint16_t UART_RxAvailable(const uart_handle_t *uart);

/**
 * @brief Handle a receive interrupt for one UART instance.
 *
 * In flat interrupt mode, call this from the application's main ISR for each UART handle
 * that might have a pending RX interrupt.
 *
 * @param uart  UART instance associated with the hardware interrupt.
 */
void UART_HandleRxInterrupt(uart_handle_t *uart);

/**
 * @brief Handle a transmit interrupt for one UART instance.
 *
 * In flat interrupt mode, call this from the application's main ISR for each UART handle
 * that might have a pending TX interrupt.
 *
 * @param uart  UART instance associated with the hardware interrupt.
 */
void UART_HandleTxInterrupt(uart_handle_t *uart);

/**
 * @brief Select which UART instance printf() should use through putch().
 *
 * Only one UART can be the active printf target at a time because XC8 provides one global
 * putch() hook.
 *
 * @param uart  UART instance to use for printf(), or NULL to disable printf routing.
 */
void UART_SelectPrintfTarget(uart_handle_t *uart);

/**
 * @brief XC8 printf hook used by printf(), puts(), and putchar().
 *
 * This writes through the currently selected printf target set by UART_SelectPrintfTarget().
 *
 * @param data  Character emitted by the C runtime.
 */
void putch(char data);

#endif /* UARTLIB_H */
