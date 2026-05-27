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
#include "config.h"
#include "app.h"
#include "../../Libraries/UARTLIB/uartlib.h"
//#include "../../Libraries/I2CLIB/i2clib.h"
#include "i2clib.h"

static char console_tx_buffer[128];
static char console_rx_buffer[128];
uint8_t i2c_buffer[16];

uart_handle_t console_uart = {
    .port = UART_PORT_1,
    .high_speed_baud = false,
    .baud_rate = 19200U,
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

i2c_handle_t i2c_host = {
     .speed_khz = 100,
     .channel = 1,
     .mode = I2C_MODE_HOST_7BIT,
     .retry_count = 3
};

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
    
     APP_Initialize();

     while (1)
     {
          APP_Service();

          while (UART_RxAvailable(&console_uart) > 0U)
          {
               if (UART_ReadChar(&console_uart, &rx_char))
               {
                    (void)UART_WriteChar(&console_uart, rx_char);
               }
          }
     }
}

