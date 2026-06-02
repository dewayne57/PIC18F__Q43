#if !defined(UART_ISR_MODE_T)
typedef enum {
    UART_ISR_FLAT = 0,     /**< Reserved for application policy (library no longer defines ISR functions) */
    UART_ISR_VECTORED = 1  /**< Reserved for application policy (library no longer defines ISR functions) */
} uart_isr_mode_t;
#define UART_ISR_MODE_T
#endif
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
 *      TX/RX buffers, including TX/RX (and optional CTS/RTS) PPS pin fields.
 *   3. The library configures TRIS and PPS during UART_Open().
 *   4. Call UART_Open(&handle).
 *   5. In application-owned ISR code, call UART_HandleRxInterrupt(&handle),
 *      UART_HandleTxInterrupt(&handle), or UART_HandleInterrupts(&handle).
 *
 * ***************************************************************************************** */

#ifndef UARTLIB_H
#define UARTLIB_H

#include <stdbool.h>
#include <stdint.h>

#define UART_STATUS_MESSAGE_MAX_BYTES 40U

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
 * @brief PPS pin selector used for UART TX/RX/CTS/RTS mapping.
 *
 * UART_PPS_PIN_NONE is zero so omitted designated initializers stay safe.
 * For valid pins, the library converts enum values to UxRXPPS/UxCTSPPS codes internally.
 */
typedef enum
{
    UART_PPS_PIN_NONE = 0x00,

    UART_PPS_PIN_RA0 = 0x01,
    UART_PPS_PIN_RA1 = 0x02,
    UART_PPS_PIN_RA2 = 0x03,
    UART_PPS_PIN_RA3 = 0x04,
    UART_PPS_PIN_RA4 = 0x05,
    UART_PPS_PIN_RA5 = 0x06,
    UART_PPS_PIN_RA6 = 0x07,
    UART_PPS_PIN_RA7 = 0x08,

    UART_PPS_PIN_RB0 = 0x09,
    UART_PPS_PIN_RB1 = 0x0A,
    UART_PPS_PIN_RB2 = 0x0B,
    UART_PPS_PIN_RB3 = 0x0C,
    UART_PPS_PIN_RB4 = 0x0D,
    UART_PPS_PIN_RB5 = 0x0E,
    UART_PPS_PIN_RB6 = 0x0F,
    UART_PPS_PIN_RB7 = 0x10,

    UART_PPS_PIN_RC0 = 0x11,
    UART_PPS_PIN_RC1 = 0x12,
    UART_PPS_PIN_RC2 = 0x13,
    UART_PPS_PIN_RC3 = 0x14,
    UART_PPS_PIN_RC4 = 0x15,
    UART_PPS_PIN_RC5 = 0x16,
    UART_PPS_PIN_RC6 = 0x17,
    UART_PPS_PIN_RC7 = 0x18,

    UART_PPS_PIN_RD0 = 0x19,
    UART_PPS_PIN_RD1 = 0x1A,
    UART_PPS_PIN_RD2 = 0x1B,
    UART_PPS_PIN_RD3 = 0x1C,
    UART_PPS_PIN_RD4 = 0x1D,
    UART_PPS_PIN_RD5 = 0x1E,
    UART_PPS_PIN_RD6 = 0x1F,
    UART_PPS_PIN_RD7 = 0x20,

    UART_PPS_PIN_RE0 = 0x21,
    UART_PPS_PIN_RE1 = 0x22,
    UART_PPS_PIN_RE2 = 0x23,
    UART_PPS_PIN_RE3 = 0x24
} uart_pps_pin_t;

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
    uart_pps_pin_t    tx_pin;           /**< TX output pin mapping (required) */
    uart_pps_pin_t    rx_pin;           /**< RX input pin mapping (required) */
    uart_pps_pin_t    rts_pin;          /**< RTS output pin mapping (required for RTS/CTS flow) */
    uart_pps_pin_t    cts_pin;          /**< CTS input pin mapping (required for RTS/CTS flow) */
    uart_isr_mode_t   isr_mode;         /**< Interrupt mode: flat or vectored. Determines ISR wiring. */
    char             *tx_buffer;        /**< Application-owned transmit ring buffer */
    char             *rx_buffer;        /**< Application-owned receive ring buffer */

    /* The user must not set, only initially clear, the following fields. */
    bool              initialized;      /**< Open/closed state: true=open, false=closed */
    volatile uint16_t tx_head;          /**< Next write slot in the transmit buffer */
    volatile uint16_t tx_tail;          /**< Next read slot in the transmit buffer */
    volatile uint16_t rx_head;          /**< Next write slot in the receive buffer */
    volatile uint16_t rx_tail;          /**< Next read slot in the receive buffer */
    char              status_message[UART_STATUS_MESSAGE_MAX_BYTES]; /**< Diagnostic status message storage (null-terminated) */
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
 * @brief Store a diagnostic status message in the UART handle.
 *
 * The message is copied into uart->status_message using bounded copy. The buffer is always
 * null-terminated. A NULL message clears the stored status string.
 *
 * @param uart     UART instance to update.
 * @param message  Source C string to store, or NULL to clear.
 * @return true if uart is valid and the status field was updated, false otherwise.
 */
bool UART_SetStatusMessage(uart_handle_t *uart, const char *message);

/**
 * @brief Get the current diagnostic status message for a UART handle.
 *
 * @param uart  UART instance to query.
 * @return Pointer to the stored null-terminated status string, or an empty string when uart is NULL.
 */
const char *UART_GetStatusMessage(const uart_handle_t *uart);

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
 * @brief Handle both receive and transmit interrupt sources for one UART instance.
 *
 * This is a convenience wrapper for application-owned ISRs and calls
 * UART_HandleRxInterrupt(uart) followed by UART_HandleTxInterrupt(uart).
 *
 * @param uart  UART instance associated with the hardware interrupt source(s).
 */
void UART_HandleInterrupts(uart_handle_t *uart);

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
