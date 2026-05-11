/* *****************************************************************************************
 *   File Name: uartlib.c
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
 *   The library keeps UART configuration and ring-buffer state in application-owned
 *   uart_handle_t structures.  The application chooses which UART peripheral each
 *   handle targets, provides the TX and RX buffers, and passes the handle to the API.
 *
 *   This design allows one project to run multiple UART peripherals at once without
 *   duplicating driver code or maintaining one set of global state per UART module.
 *
 * ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "uartlib.h"

static uart_handle_t *uart_instances[5] = { (uart_handle_t *)0 };
static uart_handle_t *uart_printf_target = (uart_handle_t *)0;

/// @brief Return true when value is a power of 2 and large enough for a ring buffer.
static bool UART_IsPowerOfTwo(uint16_t value)
{
    return ((value >= 2U) && ((value & (value - 1U)) == 0U));
}

/// @brief Convert UART port number to a 0-based array index.
static uint8_t UART_PortIndex(uart_port_t port)
{
    return (uint8_t)((uint8_t)port - 1U);
}

/// @brief Return true if the port number is in the supported UART1-UART5 range.
static bool UART_PortIsValid(uart_port_t port)
{
    return ((port >= UART_PORT_1) && (port <= UART_PORT_5));
}

/// @brief Return true if the parity enumeration contains a supported value.
static bool UART_ParityIsValid(uart_parity_t parity)
{
    return ((parity == UART_PARITY_NONE) || (parity == UART_PARITY_ODD) || (parity == UART_PARITY_EVEN));
}

/// @brief Return true if the stop-bit enumeration contains a supported value.
static bool UART_StopBitsAreValid(uart_stop_bits_t stop_bits)
{
    return ((stop_bits == UART_STOP_BITS_1) || (stop_bits == UART_STOP_BITS_1_5) || (stop_bits == UART_STOP_BITS_2));
}

/// @brief Return true if the flow-control enumeration contains a supported value.
static bool UART_FlowControlIsValid(uart_flow_t flow_control)
{
    return ((flow_control == UART_FLOW_NONE) || (flow_control == UART_FLOW_XON_XOFF) || (flow_control == UART_FLOW_RTS_CTS));
}

/// @brief Validate an application-owned UART handle before touching hardware.
static bool UART_HandleIsValid(const uart_handle_t *uart)
{
    if (uart == (const uart_handle_t *)0)
    {
        return false;
    }

    if (!UART_PortIsValid(uart->port))
    {
        return false;
    }

    if ((uart->tx_buffer == (char *)0) || (uart->rx_buffer == (char *)0))
    {
        return false;
    }

    if ((!UART_IsPowerOfTwo(uart->tx_buffer_size)) || (!UART_IsPowerOfTwo(uart->rx_buffer_size)))
    {
        return false;
    }

    if ((uart->data_bits != 8U) && (uart->data_bits != 9U))
    {
        return false;
    }

    if ((!UART_ParityIsValid(uart->parity)) || (!UART_StopBitsAreValid(uart->stop_bits)) || (!UART_FlowControlIsValid(uart->flow_control)))
    {
        return false;
    }

    if ((uart->port != UART_PORT_1) && (uart->flow_control != UART_FLOW_NONE))
    {
        return false;
    }

    return true;
}

/// @brief Return the framing mode value programmed into UxCON0.MODE.
static uint8_t UART_FrameModeValue(const uart_handle_t *uart)
{
    if (uart->data_bits == 9U)
    {
        return 0x2U;
    }

    if (uart->parity == UART_PARITY_ODD)
    {
        return 0x8U;
    }

    if (uart->parity == UART_PARITY_EVEN)
    {
        return 0x9U;
    }

    return 0x0U;
}

/// @brief Return the stop-bit field value programmed into UxCON2.STP.
static uint8_t UART_StopBitFieldValue(const uart_handle_t *uart)
{
    if ((uart->stop_bits == UART_STOP_BITS_1_5) || (uart->stop_bits == UART_STOP_BITS_2))
    {
        return (uint8_t)uart->stop_bits;
    }

    return 0U;
}

/// @brief Reset the selected UART control registers before initialization.
static void UART_ResetHardware(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        U1CON0 = 0x00U;
        U1CON1 = 0x00U;
        U1CON2 = 0x00U;
        break;

    case UART_PORT_2:
        U2CON0 = 0x00U;
        U2CON1 = 0x00U;
        U2CON2 = 0x00U;
        break;

    case UART_PORT_3:
        U3CON0 = 0x00U;
        U3CON1 = 0x00U;
        U3CON2 = 0x00U;
        break;

    case UART_PORT_4:
        U4CON0 = 0x00U;
        U4CON1 = 0x00U;
        U4CON2 = 0x00U;
        break;

    case UART_PORT_5:
        U5CON0 = 0x00U;
        U5CON1 = 0x00U;
        U5CON2 = 0x00U;
        break;

    default:
        break;
    }
}

/// @brief Apply framing, stop-bit, and flow-control settings to the selected UART.
static void UART_ApplyConfiguration(const uart_handle_t *uart)
{
    uint8_t mode_value = UART_FrameModeValue(uart);
    uint8_t stop_value = UART_StopBitFieldValue(uart);
    uint8_t flow_value = (uint8_t)uart->flow_control;
    uint8_t brgs_value = (uart->high_speed_baud ? 1U : 0U);

    switch (uart->port)
    {
    case UART_PORT_1:
        U1CON0bits.MODE = mode_value;
        U1CON0bits.BRGS = brgs_value;
        U1CON2bits.STP = stop_value;
        U1CON2bits.FLO = flow_value;
        U1CON2bits.RUNOVF = 1U;
        break;

    case UART_PORT_2:
        U2CON0bits.MODE = mode_value;
        U2CON0bits.BRGS = brgs_value;
        U2CON2bits.STP = stop_value;
        U2CON2bits.FLO = 0U;
        U2CON2bits.RUNOVF = 1U;
        break;

    case UART_PORT_3:
        U3CON0bits.MODE = mode_value;
        U3CON0bits.BRGS = brgs_value;
        U3CON2bits.STP = stop_value;
        U3CON2bits.FLO = 0U;
        U3CON2bits.RUNOVF = 1U;
        break;

    case UART_PORT_4:
        U4CON0bits.MODE = mode_value;
        U4CON0bits.BRGS = brgs_value;
        U4CON2bits.STP = stop_value;
        U4CON2bits.FLO = 0U;
        U4CON2bits.RUNOVF = 1U;
        break;

    case UART_PORT_5:
        U5CON0bits.MODE = mode_value;
        U5CON0bits.BRGS = brgs_value;
        U5CON2bits.STP = stop_value;
        U5CON2bits.FLO = 0U;
        U5CON2bits.RUNOVF = 1U;
        break;

    default:
        break;
    }
}

/// @brief Program the baud-rate divisor for the selected UART.
static void UART_SetBaudRate(uart_port_t port, uint16_t brg_value)
{
    switch (port)
    {
    case UART_PORT_1:
        U1BRG = brg_value;
        break;

    case UART_PORT_2:
        U2BRG = brg_value;
        break;

    case UART_PORT_3:
        U3BRG = brg_value;
        break;

    case UART_PORT_4:
        U4BRG = brg_value;
        break;

    case UART_PORT_5:
        U5BRG = brg_value;
        break;

    default:
        break;
    }
}

/// @brief Clear pending UART error flags before enabling the selected module.
static void UART_ClearErrors(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        U1ERRIR = 0x00U;
        break;

    case UART_PORT_2:
        U2ERRIR = 0x00U;
        break;

    case UART_PORT_3:
        U3ERRIR = 0x00U;
        break;

    case UART_PORT_4:
        U4ERRIR = 0x00U;
        break;

    case UART_PORT_5:
        U5ERRIR = 0x00U;
        break;

    default:
        break;
    }
}

/// @brief Enable the TX shift-register-empty interrupt source for the selected UART.
static void UART_EnableShiftEmptyInterrupt(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        U1ERRIEbits.U1TXMTIE = 1U;
        break;

    case UART_PORT_2:
        U2ERRIEbits.U2TXMTIE = 1U;
        break;

    case UART_PORT_3:
        U3ERRIEbits.U3TXMTIE = 1U;
        break;

    case UART_PORT_4:
        U4ERRIEbits.U4TXMTIE = 1U;
        break;

    case UART_PORT_5:
        U5ERRIEbits.U5TXMTIE = 1U;
        break;

    default:
        break;
    }
}

/// @brief Enable or disable the selected UART receive interrupt.
static void UART_SetRxInterruptEnabled(uart_port_t port, bool enabled)
{
    switch (port)
    {
    case UART_PORT_1:
        PIE4bits.U1RXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_2:
        PIE8bits.U2RXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_3:
        PIE9bits.U3RXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_4:
        PIE12bits.U4RXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_5:
        PIE13bits.U5RXIE = (enabled ? 1U : 0U);
        break;

    default:
        break;
    }
}

/// @brief Enable or disable the selected UART transmit interrupt.
static void UART_SetTxInterruptEnabled(uart_port_t port, bool enabled)
{
    switch (port)
    {
    case UART_PORT_1:
        PIE4bits.U1TXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_2:
        PIE8bits.U2TXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_3:
        PIE9bits.U3TXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_4:
        PIE12bits.U4TXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_5:
        PIE13bits.U5TXIE = (enabled ? 1U : 0U);
        break;

    default:
        break;
    }
}

/// @brief Return true when the selected UART receive interrupt is enabled and pending.
static bool UART_RxInterruptIsPending(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        return ((PIE4bits.U1RXIE != 0U) && (PIR4bits.U1RXIF != 0U));

    case UART_PORT_2:
        return ((PIE8bits.U2RXIE != 0U) && (PIR8bits.U2RXIF != 0U));

    case UART_PORT_3:
        return ((PIE9bits.U3RXIE != 0U) && (PIR9bits.U3RXIF != 0U));

    case UART_PORT_4:
        return ((PIE12bits.U4RXIE != 0U) && (PIR12bits.U4RXIF != 0U));

    case UART_PORT_5:
        return ((PIE13bits.U5RXIE != 0U) && (PIR13bits.U5RXIF != 0U));

    default:
        return false;
    }
}

/// @brief Return true when the selected UART transmit interrupt is enabled and pending.
static bool UART_TxInterruptIsPending(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        return ((PIE4bits.U1TXIE != 0U) && (PIR4bits.U1TXIF != 0U));

    case UART_PORT_2:
        return ((PIE8bits.U2TXIE != 0U) && (PIR8bits.U2TXIF != 0U));

    case UART_PORT_3:
        return ((PIE9bits.U3TXIE != 0U) && (PIR9bits.U3TXIF != 0U));

    case UART_PORT_4:
        return ((PIE12bits.U4TXIE != 0U) && (PIR12bits.U4TXIF != 0U));

    case UART_PORT_5:
        return ((PIE13bits.U5TXIE != 0U) && (PIR13bits.U5TXIF != 0U));

    default:
        return false;
    }
}

/// @brief Enable TX and RX circuitry, then turn the selected UART module on.
static void UART_EnableHardware(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        U1CON0bits.TXEN = 1U;
        U1CON0bits.RXEN = 1U;
        U1CON1bits.ON = 1U;
        break;

    case UART_PORT_2:
        U2CON0bits.TXEN = 1U;
        U2CON0bits.RXEN = 1U;
        U2CON1bits.ON = 1U;
        break;

    case UART_PORT_3:
        U3CON0bits.TXEN = 1U;
        U3CON0bits.RXEN = 1U;
        U3CON1bits.ON = 1U;
        break;

    case UART_PORT_4:
        U4CON0bits.TXEN = 1U;
        U4CON0bits.RXEN = 1U;
        U4CON1bits.ON = 1U;
        break;

    case UART_PORT_5:
        U5CON0bits.TXEN = 1U;
        U5CON0bits.RXEN = 1U;
        U5CON1bits.ON = 1U;
        break;

    default:
        break;
    }
}

/// @brief Disable TX/RX circuitry and turn the selected UART module off.
static void UART_DisableHardware(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        U1CON0bits.TXEN = 0U;
        U1CON0bits.RXEN = 0U;
        U1CON1bits.ON = 0U;
        break;

    case UART_PORT_2:
        U2CON0bits.TXEN = 0U;
        U2CON0bits.RXEN = 0U;
        U2CON1bits.ON = 0U;
        break;

    case UART_PORT_3:
        U3CON0bits.TXEN = 0U;
        U3CON0bits.RXEN = 0U;
        U3CON1bits.ON = 0U;
        break;

    case UART_PORT_4:
        U4CON0bits.TXEN = 0U;
        U4CON0bits.RXEN = 0U;
        U4CON1bits.ON = 0U;
        break;

    case UART_PORT_5:
        U5CON0bits.TXEN = 0U;
        U5CON0bits.RXEN = 0U;
        U5CON1bits.ON = 0U;
        break;

    default:
        break;
    }
}

/// @brief Discard a startup glitch byte that may have appeared during line settling.
static void UART_DiscardStartupRxByte(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        (void)U1RXB;
        break;

    case UART_PORT_2:
        (void)U2RXB;
        break;

    case UART_PORT_3:
        (void)U3RXB;
        break;

    case UART_PORT_4:
        (void)U4RXB;
        break;

    case UART_PORT_5:
        (void)U5RXB;
        break;

    default:
        break;
    }
}

/// @brief Read one byte from the selected UART hardware receive buffer.
static uint8_t UART_ReadRxByte(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        return U1RXB;

    case UART_PORT_2:
        return U2RXB;

    case UART_PORT_3:
        return U3RXB;

    case UART_PORT_4:
        return U4RXB;

    case UART_PORT_5:
        return U5RXB;

    default:
        return 0U;
    }
}

/// @brief Write one byte into the selected UART hardware transmit buffer.
static void UART_WriteTxByte(uart_port_t port, uint8_t data)
{
    switch (port)
    {
    case UART_PORT_1:
        U1TXB = data;
        break;

    case UART_PORT_2:
        U2TXB = data;
        break;

    case UART_PORT_3:
        U3TXB = data;
        break;

    case UART_PORT_4:
        U4TXB = data;
        break;

    case UART_PORT_5:
        U5TXB = data;
        break;

    default:
        break;
    }
}

/// @brief Return true when the selected UART hardware TX FIFO is empty.
static bool UART_TxHardwareBufferEmpty(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        return (U1FIFObits.TXBE != 0U);

    case UART_PORT_2:
        return (U2FIFObits.TXBE != 0U);

    case UART_PORT_3:
        return (U3FIFObits.TXBE != 0U);

    case UART_PORT_4:
        return (U4FIFObits.TXBE != 0U);

    case UART_PORT_5:
        return (U5FIFObits.TXBE != 0U);

    default:
        return false;
    }
}

/// @brief Return the TX ring-buffer mask used for fast wraparound.
static uint16_t UART_TxBufferMask(const uart_handle_t *uart)
{
    return (uint16_t)(uart->tx_buffer_size - 1U);
}

/// @brief Return the RX ring-buffer mask used for fast wraparound.
static uint16_t UART_RxBufferMask(const uart_handle_t *uart)
{
    return (uint16_t)(uart->rx_buffer_size - 1U);
}

/// @brief Return the number of unused bytes remaining in the selected TX ring buffer.
static uint16_t UART_TxBufferFreeCount(const uart_handle_t *uart)
{
    uint16_t used;

    if (uart->tx_head >= uart->tx_tail)
    {
        used = (uint16_t)(uart->tx_head - uart->tx_tail);
    }
    else
    {
        used = (uint16_t)(uart->tx_buffer_size - (uint16_t)(uart->tx_tail - uart->tx_head));
    }

    return (uint16_t)((uart->tx_buffer_size - 1U) - used);
}

/// @brief Push one byte into the selected TX ring buffer.
static bool UART_TxBufferPush(uart_handle_t *uart, char data)
{
    uint16_t next_head = (uint16_t)((uart->tx_head + 1U) & UART_TxBufferMask(uart));

    if (next_head == uart->tx_tail)
    {
        return false;
    }

    uart->tx_buffer[uart->tx_head] = data;
    uart->tx_head = next_head;
    return true;
}

/// @brief Pop one byte from the selected TX ring buffer.
static bool UART_TxBufferPop(uart_handle_t *uart, char *data)
{
    if (uart->tx_head == uart->tx_tail)
    {
        return false;
    }

    *data = uart->tx_buffer[uart->tx_tail];
    uart->tx_tail = (uint16_t)((uart->tx_tail + 1U) & UART_TxBufferMask(uart));
    return true;
}

/// @brief Return the number of unused bytes remaining in the selected RX ring buffer.
static uint16_t UART_RxBufferFreeCount(const uart_handle_t *uart)
{
    uint16_t used;

    if (uart->rx_head >= uart->rx_tail)
    {
        used = (uint16_t)(uart->rx_head - uart->rx_tail);
    }
    else
    {
        used = (uint16_t)(uart->rx_buffer_size - (uint16_t)(uart->rx_tail - uart->rx_head));
    }

    return (uint16_t)((uart->rx_buffer_size - 1U) - used);
}

/// @brief Push one byte into the selected RX ring buffer.
static bool UART_RxBufferPush(uart_handle_t *uart, char data)
{
    uint16_t next_head;

    if (data == 0x00)
    {
        return true;
    }

    next_head = (uint16_t)((uart->rx_head + 1U) & UART_RxBufferMask(uart));
    if (next_head == uart->rx_tail)
    {
        return false;
    }

    uart->rx_buffer[uart->rx_head] = data;
    uart->rx_head = next_head;
    return true;
}

/// @brief Pop one byte from the selected RX ring buffer.
static bool UART_RxBufferPop(uart_handle_t *uart, char *data)
{
    if (uart->rx_head == uart->rx_tail)
    {
        return false;
    }

    *data = uart->rx_buffer[uart->rx_tail];
    uart->rx_tail = (uint16_t)((uart->rx_tail + 1U) & UART_RxBufferMask(uart));
    return true;
}

/// @brief Load the next queued byte into the hardware transmitter for one UART instance.
static void UART_SendNext(uart_handle_t *uart)
{
    char next;

    if (!UART_TxBufferPop(uart, &next))
    {
        UART_SetTxInterruptEnabled(uart->port, false);
        return;
    }

    UART_WriteTxByte(uart->port, (uint8_t)next);
    UART_SetTxInterruptEnabled(uart->port, true);
}

/// @brief Open one UART instance described by an application-owned handle.
bool UART_Open(uart_handle_t *uart)
{
    if ((uart != (uart_handle_t *)0) && (uart->initialized))
    {
        return true;
    }

    if (!UART_HandleIsValid(uart))
    {
        return false;
    }

    uart->tx_head = 0U;
    uart->tx_tail = 0U;
    uart->rx_head = 0U;
    uart->rx_tail = 0U;
    uart->initialized = false;

    UART_ResetHardware(uart->port);
    UART_ApplyConfiguration(uart);
    UART_SetBaudRate(uart->port, uart->brg_value);
    UART_ClearErrors(uart->port);
    UART_EnableShiftEmptyInterrupt(uart->port);
    UART_SetTxInterruptEnabled(uart->port, false);
    UART_SetRxInterruptEnabled(uart->port, false);
    UART_EnableHardware(uart->port);

    /* Startup settling delay — avoids __delay_ms() dependency on _XTAL_FREQ. */
    { volatile uint16_t i = 0xFFFFU; while (i-- != 0U) { } }
    UART_DiscardStartupRxByte(uart->port);

    uart_instances[UART_PortIndex(uart->port)] = uart;
    UART_SetRxInterruptEnabled(uart->port, true);
    uart->initialized = true;
    return true;
}

/// @brief Close one UART instance so the application can safely reconfigure and reopen it.
void UART_Close(uart_handle_t *uart)
{
    uint8_t index;

    if (uart == (uart_handle_t *)0)
    {
        return;
    }

    if (!uart->initialized)
    {
        return;
    }

    if (uart_printf_target == uart)
    {
        uart_printf_target = (uart_handle_t *)0;
    }

    uart->initialized = false;

    if (!UART_PortIsValid(uart->port))
    {
        uart->tx_head = 0U;
        uart->tx_tail = 0U;
        uart->rx_head = 0U;
        uart->rx_tail = 0U;
        return;
    }

    UART_SetRxInterruptEnabled(uart->port, false);
    UART_SetTxInterruptEnabled(uart->port, false);
    UART_DisableHardware(uart->port);

    index = UART_PortIndex(uart->port);
    if (uart_instances[index] == uart)
    {
        uart_instances[index] = (uart_handle_t *)0;
    }

    uart->tx_head = 0U;
    uart->tx_tail = 0U;
    uart->rx_head = 0U;
    uart->rx_tail = 0U;
}

/// @brief Queue one character for transmission on the selected UART.
bool UART_WriteChar(uart_handle_t *uart, char data)
{
    if ((uart == (uart_handle_t *)0) || (!uart->initialized))
    {
        return false;
    }

    while (UART_TxBufferFreeCount(uart) == 0U)
    {
    }

    (void)UART_TxBufferPush(uart, data);

    if (UART_TxHardwareBufferEmpty(uart->port) && (!UART_TxInterruptIsPending(uart->port)))
    {
        UART_SendNext(uart);
    }

    return true;
}

/// @brief Read one character from the selected UART receive buffer.
bool UART_ReadChar(uart_handle_t *uart, char *data)
{
    if ((uart == (uart_handle_t *)0) || (data == (char *)0) || (!uart->initialized))
    {
        return false;
    }

    if (!UART_RxBufferPop(uart, data))
    {
        return false;
    }

    UART_SetRxInterruptEnabled(uart->port, true);
    return true;
}

/// @brief Return the number of bytes currently buffered in the selected UART receive queue.
uint16_t UART_RxAvailable(const uart_handle_t *uart)
{
    if ((uart == (const uart_handle_t *)0) || (!uart->initialized))
    {
        return 0U;
    }

    if (uart->rx_head >= uart->rx_tail)
    {
        return (uint16_t)(uart->rx_head - uart->rx_tail);
    }

    return (uint16_t)(uart->rx_buffer_size - (uint16_t)(uart->rx_tail - uart->rx_head));
}

/// @brief Handle a receive interrupt for one UART instance.
void UART_HandleRxInterrupt(uart_handle_t *uart)
{
    if ((uart == (uart_handle_t *)0) || (!uart->initialized))
    {
        return;
    }

    if (UART_RxInterruptIsPending(uart->port))
    {
        if (UART_RxBufferFreeCount(uart) == 0U)
        {
            (void)UART_ReadRxByte(uart->port);
            UART_SetRxInterruptEnabled(uart->port, false);
        }
        else
        {
            (void)UART_RxBufferPush(uart, (char)UART_ReadRxByte(uart->port));
        }
    }
}

/// @brief Handle a transmit interrupt for one UART instance.
void UART_HandleTxInterrupt(uart_handle_t *uart)
{
    if ((uart == (uart_handle_t *)0) || (!uart->initialized))
    {
        return;
    }

    if (UART_TxInterruptIsPending(uart->port))
    {
        UART_SendNext(uart);
    }
}

/// @brief Select which UART instance the global printf hook should use.
void UART_SelectPrintfTarget(uart_handle_t *uart)
{
    if ((uart != (uart_handle_t *)0) && (!uart->initialized))
    {
        return;
    }

    uart_printf_target = uart;
}

/// @brief XC8 printf retarget hook - routes output through the selected UART instance.
void putch(char data)
{
    if (uart_printf_target != (uart_handle_t *)0)
    {
        (void)UART_WriteChar(uart_printf_target, data);
    }
}

#ifdef UART_VECTORED_INTERRUPTS
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
{
    UART_HandleRxInterrupt(uart_instances[0]);
}

void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
{
    UART_HandleTxInterrupt(uart_instances[0]);
}

void __interrupt(irq(IRQ_U2RX), low_priority) UART2_RX_ISR(void)
{
    UART_HandleRxInterrupt(uart_instances[1]);
}

void __interrupt(irq(IRQ_U2TX), low_priority) UART2_TX_ISR(void)
{
    UART_HandleTxInterrupt(uart_instances[1]);
}

void __interrupt(irq(IRQ_U3RX), low_priority) UART3_RX_ISR(void)
{
    UART_HandleRxInterrupt(uart_instances[2]);
}

void __interrupt(irq(IRQ_U3TX), low_priority) UART3_TX_ISR(void)
{
    UART_HandleTxInterrupt(uart_instances[2]);
}

void __interrupt(irq(IRQ_U4RX), low_priority) UART4_RX_ISR(void)
{
    UART_HandleRxInterrupt(uart_instances[3]);
}

void __interrupt(irq(IRQ_U4TX), low_priority) UART4_TX_ISR(void)
{
    UART_HandleTxInterrupt(uart_instances[3]);
}

void __interrupt(irq(IRQ_U5RX), low_priority) UART5_RX_ISR(void)
{
    UART_HandleRxInterrupt(uart_instances[4]);
}

void __interrupt(irq(IRQ_U5TX), low_priority) UART5_TX_ISR(void)
{
    UART_HandleTxInterrupt(uart_instances[4]);
}
#endif
