/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Hardware I2C module host implementation for PIC18F47Q43.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-19
 *
 *   Copyright (c) 2026, Dewayne Hafenstein.
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "i2c.h"

#include "../../common/mcp23x17.h"
#include "../../Libraries/UARTLIB/uartlib.h"
// #include "../../Libraries/I2CLIB/i2clib.h"

static I2C_Status_t MCP23017_Initialize(void); 

static char console_tx_buffer[128];
static char console_rx_buffer[128];

uart_handle_t console_uart = {.port = UART_PORT_1,
                              .high_speed_baud = false,
                              .baud_rate = 57600U,
                              .fosc = _XTAL_FREQ,
                              .data_bits = 8U,
                              .parity = UART_PARITY_NONE,
                              .stop_bits = UART_STOP_BITS_1,
                              .flow_control = UART_FLOW_NONE,
                              .tx_pin = UART_PPS_PIN_RB0,
                              .rx_pin = UART_PPS_PIN_RB1,
                              .rts_pin = UART_PPS_PIN_NONE,
                              .cts_pin = UART_PPS_PIN_NONE,
                              .isr_mode = UART_ISR_VECTORED,
                              .tx_buffer = console_tx_buffer,
                              .tx_buffer_size = sizeof(console_tx_buffer),
                              .rx_buffer = console_rx_buffer,
                              .rx_buffer_size = sizeof(console_rx_buffer),
                              .tx_head = 0U,
                              .tx_tail = 0U,
                              .rx_head = 0U,
                              .rx_tail = 0U,
                              .initialized = false};

// @brief I2C host handle for the demonstration project.
I2C_Handle_t i2c_host;
uint8_t i2c_buffer[16]; // Buffer for I2C transactions
volatile uint8_t s_last_porta_value = 0x00; // Last known value of MCP23017 Port A
volatile bool s_porta_value_changed = false; // Flag indicating if the last known value of Port A has changed
volatile bool s_porta_read_pending = false;  // Async GPIOA read started by INT1 ISR

/// @brief Main application entry point.
/// @param  None
/// @return None
/// @note This application initializes the system, UART1 console output, and the I2C1 hardware
///       module host, then enters an infinite loop. The I2C host is available for I2C
///       transactions via the i2c_host handle. External interrupt on RB2 is monitored for
///       external events. The main loop remains responsive, allowing for I2C host operations
///       and UART communication.
void main(void)
{
    char rx_char;

    SYSTEM_Initialize();

    if (!UART_Open(&console_uart))
    {
        while (1)
        {
        }
    }

    UART_SelectPrintfTarget(&console_uart);
    printf("I2C Module Host example%s", CRLF);
    I2C_Status_t status =
        I2C_Init(&i2c_host, I2C_CLOCK_MFINTOSC, I2C_MODE_MASTER_7, 0U, NONE);
    if (status != I2C_OK)
    {
        printf("I2C initialization failed with status: %d\n", status);
        while (1)
        {
        }
    }

    status = MCP23017_Initialize();
    if (status != I2C_OK)
    {
        printf("MCP23017 initialization failed with status: %d\n", status);
        while (1)
        {
        }
    }

    printf("Reading port A%s", CRLF);
    status = I2C_ReadRegister(&i2c_host, MCP23017_ADDR, BANKED_GPIOA, (uint8_t*)&s_last_porta_value, 1U);
    if (status != I2C_OK)
    {
        printf("I2C read from MCP23017 Port A failed with status: %d%s", status, CRLF);
    }
    while ((status = I2C_IsBusy(&i2c_host)) == I2C_BUSY)
    {
    }
 
    printf("Port value was %02x%s", s_last_porta_value, CRLF);
    s_porta_value_changed = true;

    // Arm INT1 only after MCP configuration and initial GPIOA read have cleared
    // any startup interrupt condition on the active-low INT line.
    IPR6bits.INT1IP = 0; // Keep INT1 on low-priority vector (matches ISR declaration).
    PIR6bits.INT1IF = 0;
    PIE6bits.INT1IE = 1;

    while (1)
    {
        if (s_porta_read_pending)
        {
            status = I2C_IsBusy(&i2c_host);
            if (status == I2C_OK)
            {
                s_porta_read_pending = false;
                s_porta_value_changed = true;
            }
            else if (status != I2C_BUSY)
            {
                printf("I2C read completion error: %d%s", status, CRLF);
                s_porta_read_pending = false;
            }
        }

        if (s_porta_value_changed)
        {
            uint8_t port_a_value = s_last_porta_value;
            printf("MCP23017 Port A value: 0x%02X%s", port_a_value, CRLF);
            s_porta_value_changed = false;
            i2c_buffer[0] = BANKED_GPIOB; 
            i2c_buffer[1] = port_a_value; 
            status = I2C_Write(&i2c_host, MCP23017_ADDR, i2c_buffer, 2); 
            if (status != I2C_OK)
            {
                printf("I2C write to MCP23017 Port B failed with status: %d%s", status, CRLF);
            }
        }

        __delay_ms(1000);
    }
}

/// @brief UART1 RX ISR (vectored)
/// @param  None
/// @return None
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
{
    UART_HandleRxInterrupt(&console_uart);
}

/// @brief UART1 TX ISR (vectored)
/// @param  None
/// @return None
void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
{
    UART_HandleTxInterrupt(&console_uart);
}

/// @brief Handle external interrupt on RB2 to read MCP23017 port A and copy to MCP23017 port B
/// @param  None
/// @return None
/// @note This function reads the MCP23017 port A register and writes the value to MCP23017
///       port B
void __interrupt(irq(IRQ_INT1), low_priority) Extern_ISR(void)
{
    // Keep ISR short: queue a single asynchronous GPIOA read and process result in main.
    if (!s_porta_read_pending)
    {
        I2C_Status_t status =
            I2C_ReadRegister(&i2c_host, MCP23017_ADDR, BANKED_GPIOA, (uint8_t*)&s_last_porta_value, 1U);
        if (status == I2C_OK)
        {
            s_porta_read_pending = true;
        }
    }

    // Clear the INT1 interrupt flag
    PIR6bits.INT1IF = 0;
}

/// @brief Initialize the MCP23017 I/O expander.
/// The MCP23017 is configured in banked addressing mode so Port A registers are
/// contiguous within one block and Port B registers are contiguous within another.
/// @return i2c_status_t indicating success or error
static I2C_Status_t MCP23017_Initialize()
{
    uint8_t iocon = 0x84;
    I2C_Status_t status;
    
    PORTBbits.RB3 = 0;  // reset the MCP23017 
    __delay_us(100); 
    PORTBbits.RB3 = 1;  // Clear reset condition 
    __delay_us(100); 

    // Keep INT1 disabled during setup; main() enables it after initial GPIOA read.
    PIE6bits.INT1IE = 0;

    // Enter banked mode (BANK=1) while keeping address auto-increment enabled (SEQOP=0).
    // INTPOL=0 keeps INT active low; ODR=1 enables open-drain interrupt output.
    i2c_buffer[0] = IOCON;
    i2c_buffer[1] = iocon;
    status = I2C_Write(&i2c_host, MCP23017_ADDR, i2c_buffer, 2);
    if (status != I2C_OK)
    {
        return status;
    }

    while ((status = I2C_IsBusy(&i2c_host)) == I2C_BUSY)
    {
        // Wait for the I2C bus to become idle
    }
    if (status != I2C_OK)
    {
        printf("I2C bus error after writing to IOCON: %d%s", status, CRLF);
        return status;
    }

    // Configure Port A in one contiguous banked transfer.
    i2c_buffer[0] = BANKED_IODIRA;
        i2c_buffer[1] = 0xFF; // IODIRA: inputs
        i2c_buffer[2] = 0x00; // IPOLA
        i2c_buffer[3] = 0xFF; // GPINTENA
        i2c_buffer[4] = 0x00; // DEFVALA
        i2c_buffer[5] = 0x00; // INTCONA
        i2c_buffer[6] = iocon; // IOCON
        i2c_buffer[7] = 0xFF; // GPPUA
        i2c_buffer[8] = 0x00; // INTFA (write ignored)
        i2c_buffer[9] = 0x00; // INTCAPA (write ignored)
        i2c_buffer[10] = 0x00; // GPIOA
    status = I2C_Write(&i2c_host, MCP23017_ADDR, i2c_buffer, 11);
    if (status != I2C_OK)
    {
        return status;
    }
    while ((status = I2C_IsBusy(&i2c_host)) == I2C_BUSY)
    {
        // Wait for the I2C bus to become idle
    }
    if (status != I2C_OK)
    {
        printf("I2C bus error after writing to IOCON: %d%s", status, CRLF);
        return status;
    }

    // Configure Port B in one contiguous banked transfer.
    i2c_buffer[0] = BANKED_IODIRB;
        i2c_buffer[1] = 0x00; // IODIRB: outputs
        i2c_buffer[2] = 0x00; // IPOLB
        i2c_buffer[3] = 0x00; // GPINTENB
        i2c_buffer[4] = 0x00; // DEFVALB
        i2c_buffer[5] = 0x00; // INTCONB
        i2c_buffer[6] = iocon; // IOCON
        i2c_buffer[7] = 0x00; // GPPUB
        i2c_buffer[8] = 0x00; // INTFB (write ignored)
        i2c_buffer[9] = 0x00; // INTCAPB (write ignored)
        i2c_buffer[10] = 0x00; // GPIOB
    status = I2C_Write(&i2c_host, MCP23017_ADDR, i2c_buffer, 11);
    if (status != I2C_OK)
    {
        return status;
    }
    while ((status = I2C_IsBusy(&i2c_host)) == I2C_BUSY)
    {
        // Wait for the I2C bus to become idle
    }
    if (status != I2C_OK)
    {
        printf("I2C bus error after writing to IOCON: %d%s", status, CRLF);
        return status;
    }

    return status;
}

