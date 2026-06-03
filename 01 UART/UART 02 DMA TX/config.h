/* *****************************************************************************************
 *   File Name: config.h
 *   Description: Configuration header for the demonstration project. 
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
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
     ***************************************************************************************** */

#ifndef CONFIG_H
#define CONFIG_H

// Define UART frame format settings for parity
typedef enum
{
    UART_PARITY_NONE = 0,
    UART_PARITY_ODD = 1,
    UART_PARITY_EVEN = 2
} uart_parity_t;

// Define UART frame format settings for stop bits
typedef enum
{
    UART_STOP_BITS_1 = 0,
    UART_STOP_BITS_1_5 = 1,
    UART_STOP_BITS_2 = 2
} uart_stop_bits_t;

// PIC 18F47Q43 Configuration Bit Settings
#pragma config FEXTOSC = OFF           // External Oscillator mode selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_64MHZ // Power-up default value for COSC bits (HFINTOSC with 64 MHz)
#pragma config CSWEN = OFF             // Clock Switch Enable bit (Clock switching is disabled)
#pragma config FCMEN = OFF             // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor is disabled)
#pragma config MVECEN = 1              // Multi-vector Enable bit (Multi-vector enabled)
#pragma config WDTE = OFF              // Watchdog Timer Enable bits (WDT disabled)
#pragma config MCLRE = EXTMCLR         // MCLR Pin Enable bit (MCLR pin enabled, RE3 input pin disabled)
#pragma config LVP = ON                // Low-Voltage Programming Enable bit (Low-voltage programming enabled)
#pragma config XINST = OFF             // Extended Instruction Set Enable bit
#pragma config PPS1WAY = OFF           // Peripheral Pin Select one-way control bit

#define _XTAL_FREQ 64000000UL          // Define the system clock frequency for delay functions
#define BAUD_RATE 19200                // Define the baud rate for UART communication
#define UART_1_BRG_VALUE ((_XTAL_FREQ / (4UL * BAUD_RATE)) - 1) // Calculate the baud rate
                                                                            // generator value (see datasheet for details )
#define UART_1_PARITY UART_PARITY_NONE // No parity
#define UART_1_DATA_BITS 8            // 8 data bits
#define UART_1_STOP_BITS UART_STOP_BITS_1 // 1 stop bit
#define UART_BUFFER_SIZE (256 - 1)

/// @brief Initialize the system, including clock settings, pin configurations, and any
/// other necessary hardware setup for the UART demonstration project.  This function
/// should be called at the beginning of the main function to ensure that all
/// hardware is properly initialized before use.
/// @param  None
/// @return None
void SYSTEM_Initialize(void);

#endif /* CONFIG_H */
