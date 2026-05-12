/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for I2C 01 Bit Bang Master with external IOC interrupt.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-11
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "../../Libraries/UARTLIB/uartlib.h"
#include "i2c_bitbang.h"
#include "extern_ioc.h"

static char console_tx_buffer[128];
static char console_rx_buffer[128];

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

/// @brief Main application entry point.
/// @param  None
/// @return None
/// @note This application initializes the system, UART1 for debug output, and I2C bit bang master,
///       then enters an infinite loop. The I2C master is available for I2C transactions via the
///       i2c_master handle. External interrupt on RB2 is monitored for external events.
///       The main loop remains responsive, allowing for I2C master operations and UART communication.
void main(void)
{
    int counter = 0;

    // Initialize the system and UART debug channel.
    SYSTEM_Initialize();
    if (!UART_Open(&console_uart))
    {
        while (1)
        {
            // Halt here if UART initialization fails.
        }
    }
    UART_SelectPrintfTarget(&console_uart);

    printf("I2C 01 Bit Bang Master\r\n");

    // Initialize I2C bit bang master with 10us clock delay (approximately 50kHz I2C bus)
    i2c_handle_t i2c_master;
    if (I2C_Initialize(&i2c_master, 10) != I2C_SUCCESS)
    {
        printf("Error: Failed to initialize I2C\r\n");
        while (1)
        {
            // Halt here if I2C initialization fails
        }
    }
    printf("I2C initialized successfully\r\n");

    while (1)
    {
        // Perform a non-blocking check for received data and echo it back if available. This
        // allows the main application to remain responsive while still providing UART communication
        // capabilities. The use of UART1_RxAvailable ensures that we only attempt to read when data
        // is present, preventing blocking on an empty buffer.
        __delay_ms(1000);
        printf("Test %i\\r\\n", counter++);

        // I2C bit bang master is available via the i2c_master handle for I2C operations.
        // Example: I2C_Start(&i2c_master); I2C_SendByte(&i2c_master, 0xA0); etc.
    }
}

/// @brief ISR for UART1 Transmit, UART1 Receive, and External IOC on RB2
/// @param  None
/// @return None
void __interrupt() ISR(void)
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
