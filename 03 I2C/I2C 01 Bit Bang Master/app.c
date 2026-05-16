/* *****************************************************************************************
 *   File Name: app.c
 *   Description: Bit bang I2C master implementation for PIC18F47Q43.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-11
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
 *   This is the demonstration application implementation for the I2C 01 Bit Bang Master
 *   example. It initializes the system, sets up UART1 for debug output, and configures
 *   the I2C bit bang master interface.
 *
 *   This application uses an MCP23017 I2C I/O expander as a remote slave device for
 *   demonstration purposes. Port A is configured as digital inputs with weak pull-ups
 *   enabled and interrupt on change enabled.  Port A is connected to an 8-switch DIP
 *   switch to ground.  When a switch is closed, the pin is driven low.  On this switch
 *   this is denoted as "on" so we must interpret a low input as "on".
 *
 *   Port A and Port B interrupts are not strapped or pinned  together to allow for
 *   separate interrupt handling.  Port B interrupts are not used in this example.
 *
 *   Port B is configured as digital outputs and is connected to 8 LEDs.  The state of the
 *   LEDs is updated to match the outputs and drives 8 LEDs for visual feedback.
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "app.h"
#include "i2c.h"
#include "mcp23x17.h"
#include "../../Libraries/UARTLIB/uartlib.h"

static char console_tx_buffer[128];
static char console_rx_buffer[128];

// MCP23017 I2C address (assuming A2, A1, A0 = 0)
const uint8_t MCP23017_ADDR = 0x20; // (0100 000) << 1 = 0x40, then >> 1 for 7-bit addressing

uart_handle_t console_uart = {
    .port = UART_PORT_1,
    .high_speed_baud = false,
    .baud_rate = 19200U,
    .fosc = _XTAL_FREQ, // Replace with your actual peripheral clock frequency
    .data_bits = 8U,
    .parity = UART_PARITY_NONE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_control = UART_FLOW_NONE,
    .tx_buffer = console_tx_buffer,
    .tx_buffer_size = sizeof(console_tx_buffer),
    .rx_buffer = console_rx_buffer,
    .rx_buffer_size = sizeof(console_rx_buffer),
    .tx_head = 0U,
    .tx_tail = 0U,
    .rx_head = 0U,
    .rx_tail = 0U,
    .initialized = false};

i2c_handle_t i2c_master;
uint8_t i2c_buffer[16]; // Buffer for I2C read/write operations

/// @brief ISR for UART1 Transmit, UART1 Receive, and External IOC on RB2
/// @param  None
/// @return None
void __interrupt(irq(0x07), low_priority) ISR(void)
void __interrupt(irq(0x07), low_priority) ISR(void)
{
    // Handle UART1 Receive Interrupt
    if ((PIE4bits.U1RXIE != 0U) && (PIR4bits.U1RXIF != 0U))
    {
        UART_HandleRxInterrupt(&console_uart);
    }

    // Handle UART1 Transmit Interrupt
    if ((PIE4bits.U1TXIE != 0U) && (PIR4bits.U1TXIF != 0U))
    {
        UART_HandleTxInterrupt(&console_uart);
    }

    // Handle External IOC Interrupt on RB2
    if (PIR0bits.IOCIF != 0)
    {
        ExternIoc_HandleInterrupt();
    }
}

/// @brief Handle external interrupt on RB2 to read MCP23017 port A and copy to MCP23017 port B
/// @param  None
/// @return None
/// @note This function reads the MCP23017 port A register and writes the value to MCP23017
///       port B
void Extern_HandleInterrupt(void)
{
    uint8_t mcp_port_a_value = 0x00;

    // Read MCP23017 Port A GPIO register (SEQ_GPIOA = 0x12)
    i2c_status_t status = I2C_Read(&i2c_master, MCP23017_ADDR, SEQ_GPIOA, &mcp_port_a_value, 1);

    if (status == I2C_SUCCESS)
    {
        // Write MCP23017 Port A value to MCP23017 Port B OLAT register (SEQ_OLATB = 0x0D)
        status = I2C_Write(&i2c_master, MCP23017_ADDR, SEQ_OLATB, &mcp_port_a_value, 1);
    }

    // Clear the interrupt flag
    PIR0bits.IOCIF = 0;
}

/// @brief Initialize the application.
/// @param  None
/// @return None
/// @note This function initializes the system, UART1 for debug output, and I2C
///       bit bang master interface. It also prints status messages to the console.
void APP_Initialize(void)
{
    /*
     * Configure RB0 as UART1 TX and RB1 as UART1 RX
     */
    TRISBbits.TRISB0 = 0;   // RB0 is output (UART1 TX)
    ANSELBbits.ANSELB0 = 0; // RB0 is digital
    TRISBbits.TRISB1 = 1;   // RB1 is input (UART1 RX)
    ANSELBbits.ANSELB1 = 0; // RB1 is digital

    if (!UART_Open(&console_uart))
    {
        while (1)
        {
            // Halt here if UART initialization fails.
        }
    }
    UART_SelectPrintfTarget(&console_uart);

    printf("I2C 01 Bit Bang Master\r\n");

    /*
     * Configure RB2 as an external interrupt (INT0).  We will use this
     * interrupt to detect changes on the input pins of the MCP23017 I/O expander.
     * When a change is detected on the input pins, the MCP23017 will trigger an interrupt
     * on RB2, which we will handle in the ExternIoc_HandleInterrupt function.  This
     * allows us to respond to changes on the input pins without needing to continuously
     * poll them in the main loop, improving efficiency and responsiveness.
     */
    TRISBbits.TRISB2 = 1;   // RB2 is input
    ANSELBbits.ANSELB2 = 0; // RB2 is digital
    WPUBbits.WPUB2 = 1;     // Weak pull-up enabled on RB2

    // Configure INT0 interrupt
    // INTEDG = 0: Interrupt on falling edge
    // IOCIF flag is cleared when reading PORTB in the interrupt handler
    INTCONbits.INTEDG = 0; // Falling edge trigger for INT0 on RB2
    INTCONbits.IOCIE = 1;  // Enable IOC interrupt
    INTCONbits.IOCIF = 0;  // Clear the IOC interrupt flag

    // Initialize I2C bit bang master for 100kHz bus speed
    if (I2C_Initialize(&i2c_master, 100) != I2C_SUCCESS)
    {
        printf("Error: Failed to initialize I2C\r\n");
        while (1)
        {
            // Halt here if I2C initialization fails
        }
    }
    printf("I2C initialized successfully\r\n");
}
