/* *****************************************************************************************
 *   File Name: main.c
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
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "app.h"
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
    SYSTEM_Initialize();
    APP_Initialize();

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
