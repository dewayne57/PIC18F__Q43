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
#include <stdbool.h>
#include "config.h"
#include "app.h"
#include "spi.h"
#include "../../Libraries/UARTLIB/uartlib.h"
#include "../../Libraries/DEBUGLIB/debuglib.h"

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

/// @brief Initialize the CRC module.
/// This function should set up any necessary hardware or software resources needed for CRC
/// calculations.  The CRC module has the ability to operate with any CRC-16 polynomial, but
/// for this application we will be using the CRC-16-CCITT polynomial (0x1021) with an initial
/// value of 0xFFFF. The CRC module should be configured to use these settings for all CRC
/// calculations performed in this application.
/// The CRC module also has the ability to be interrupt driven so that the application does
/// not have to wait for the CRC calculation to complete before proceeding with other tasks.
/// However, for simplicity in this application we will perform the CRC calculation in a blocking
/// manner, so the CRC_Initialize function does not need to set up any interrupts for the CRC module.
/// We will not be using the scanner feature of the CRC module in this application, so there is
/// no need to configure any scanner settings in the CRC module.
/// @param  None
/// @return None
void CRC_Initialize(void)
{
     // For this application, we do not need to perform any specific initialization for the CRC module
     // since we will be implementing the CRC-16-CCITT calculation in software. However, if there were
     // any hardware-specific initialization required for a CRC peripheral, it would be performed here.
     CRCXOR = 0x1021; // Set the CRC polynomial to 0x1021 for CRC-16-CCITT
     CRCCON0 = 0x00; // Configure CRC control register
     CRCCON0bits.CRCEN = 1; // Enable the CRC module
     CRCCON1 = 0xFF; // data and polynomials are 16 bits in length
}

/// @brief Use the CRC module to calculate a CRC-16-CCITT value for the given message buffer and length.
/// We do not want to include the message termination characters (CR + LF or LF + CR) in the CRC
/// calculation, so we will only pass the message buffer and length up to but not including the
/// termination characters.
/// @note The CRC-16-CCITT polynomial is 0x1021 and the initial value is 0xFFFF. The CRC calculation
/// should be performed according to the standard CRC-16-CCITT algorithm, which involves iterating over
/// each byte of the message and updating the CRC value based on the current byte and the previous CRC
/// value.
/// @note The CRC calculation should be implemented in a blocking manner, meaning that the function will
/// not return until the CRC calculation is complete and the final CRC value is ready to be returned.
/// This is acceptable for this application since the CRC calculation should be relatively fast and we do
/// not have any strict real-time requirements that would necessitate an interrupt-driven approach for the
///  CRC calculation. However, in a more complex application with higher performance requirements, it may 
/// be beneficial to implement the CRC calculation in an interrupt-driven manner.
/// @note The CRC calculation is performed on 16-bit chunks of input data.  If the input message is 
/// an odd number of bytes, the last byte should be padded with a zero byte to form a complete 16-bit 
/// chunk.     
/// @param message The message buffer for which to calculate the CRC-16-CCITT value.
/// @param message_len The message length in bytes for which to calculate the CRC-16-CCITT value.
/// @return  The calculated CRC-16-CCITT value for the given message buffer and length.
uint16_t CRC16_Calculate(uint8_t *message, size_t message_len)
{
     // Ensure message is not null, has at least 3 bytes (to exclude CR + LF), and does not exceed
     // the buffer size
     if (message == NULL || message_len <= 2 || message_len > sizeof(message))
     {
          return 0xFFFF; // Return the initial CRC value if the message is null or empty
     }

     size_t effective_len = message_len - 2; // Less the CR+LF/LF+CR termination characters
     if (effective_len % 2 != 0)
     {
          // If the effective message length is odd, we need to pad it with a zero byte for CRC calculation
          message[effective_len] = 0x00; // Pad with zero byte
          effective_len++;              // Increment effective length to account for padding
     }

     CRCACC = 0xFFFF; // Initial CRC value
     for (size_t i = 0; i < effective_len; i+=2)
     {
          uint16_t word = ((uint16_t)message[i] << 8) | message[i + 1]; // Combine two bytes into a 16-bit word
          CRCDATA = word; // Load the word into the CRC data register to perform the calculation
          CRCCON0bits.CRCGO = 1; // Start the CRC calculation for the current word
          while (!CRCCON0bits.BUSY) 
          {
               // Wait for the CRC calculation to complete
          }
     }
     return CRCACC; // Return the final CRC value from the CRC accumulator register
}

/// @brief Main application entry point.
/// @param  None
/// @return None
/// @note This application initializes the system, UART1 console output, and the CRC module.
/// All data streams received on uart1 up to a cr + lf (or lf + cr) are processed to generate
/// a CRC-16-CCITT value which is then printed to the console along with the received data.
void main(void)
{
     char prev_char = '\0';
     char rx_char = '\0';
     char message[512];
     size_t message_len;

     SYSTEM_Initialize();
     CRC_Initialize();

     if (!UART_Open(&console_uart))
     {
          while (1)
          {
          }
     }

     UART_SelectPrintfTarget(&console_uart);
     APP_Initialize();
     message_len = 0;

     while (1)
     {
          APP_Service();

          while (UART_RxAvailable(&console_uart) > 0U)
          {
               if (UART_ReadChar(&console_uart, &rx_char))
               {
                    // Accumulate the received character into the message buffer until we
                    // recognize a message termination sequence (CR + LF or LF + CR). We
                    // also need to ensure that we do not exceed the bounds of the message
                    // buffer when appending characters.
                    if (message_len < sizeof(message) - 1U)
                    {
                         message[message_len] = rx_char; // Append the character to the message buffer
                         message_len++;                  // increment the message length
                         message[message_len] = '\0';    // Null-terminate the message
                    }
                    else
                    {
                         printf("Received message is too long to process.\r\n");
                         message_len = 0; // Reset message length to start fresh
                         continue;        // Skip CRC calculation for this message
                    }

                    if (prev_char == '\0')
                    {
                         prev_char = rx_char; // Store the current character as the previous character for the
                         // next iteration
                    }
                    else if ((prev_char == '\r' && rx_char == '\n') || (prev_char == '\n' && rx_char == '\r'))
                    {
                         uint16_t crc = CRC16_Calculate((uint8_t *)message, message_len);
                         printf("Received: %s | CRC-16-CCITT: 0x%04X\r\n", message, crc);
                         message_len = 0;  // Reset message length for the next message
                         prev_char = '\0'; // Reset previous character
                    }
               }
          }
     }
}
