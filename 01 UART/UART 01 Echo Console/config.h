/* *****************************************************************************************
 *   File Name: config.h
 *   Description: Configuation settings for the demonstration project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-09
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
#pragma config FEXTOSC = OFF          // Dont use the external oscillator
#pragma config RSTOSC = HFINTOSC_64MHZ // Use internal 64MHz high frequency osc
#pragma config CSWEN = OFF            // No clock switching allowed
#pragma config FCMEN = OFF            // Fail-safe clock monitor is disabled
#pragma config PR1WAY = 0             // PRLOCKED Set/Cleared repeatedly
#pragma config CLKOUTEN = 0           // Clock out is enabled on RA6
#pragma config BOREN = 3              // Brown-out reset is enabled
#pragma config LPBOREN = OFF          // Low power brown-out reset is disabled
#pragma config IVT1WAY = 0            // IVTLOCK Set/cleared repeatedly
#pragma config MVECEN = 0             // Vectored interrupts disabled <== Note: This is required
                                      // for single level interrupts
#pragma config PWRTS = 2              // Power up timer at 64mS
#pragma config MCLRE = 1              // Master clear retains that function
#pragma config XINST = OFF            // No extended instruction set
#pragma config LVP = 1                // Low voltage programming is enabled
#pragma config STVREN = ON            // Stack over/under flow causes reset
#pragma config PPS1WAY = 0            // PPSLOCK set/reset repeatedly
#pragma config ZCD = 1                // Zero-cross detection is disabled
#pragma config BORV = 0               // Brown-out voltage is set to 2.85V
#pragma config WDTE = OFF             // No watch dog timer
#pragma config SAFEN = OFF            // Storage area flash is disabled
#pragma config BBEN = OFF             // Boot block is disabled
#pragma config WRTAPP = OFF           // Application block is NOT write protected
#pragma config WRTSAF = OFF           // SAF area is not write protected
#pragma config WRTC = OFF             // Configuration registers are NOT write protected
#pragma config WRTB = OFF             // Boot block is not write protected
#pragma config WRTD = OFF             // Data EEPROM is not write protected
#pragma config CP = OFF               // Code is not protected

#define _XTAL_FREQ 64000000UL         // Define the system clock frequency for delay functions
#define BAUD_RATE 19200               // Define the baud rate for UART communication
#define UART_1_BRG_VALUE ((_XTAL_FREQ / (16UL * BAUD_RATE)) - 1) // Calculate the baud rate
                                      // generator value (see datasheet for details )
#define UART_1_PARITY UART_PARITY_NONE // No parity
#define UART_1_DATA_BITS 8            // 8 data bits 
#define UART_1_STOP_BITS UART_STOP_BITS_1 // 1 stop bit

/// @brief Initialize the system, including clock settings, pin configurations, and any 
/// other necessary hardware setup for the UART demonstration project.  This function 
/// should be called at the beginning of the main function to ensure that all
/// hardware is properly initialized before use.  
/// @param  None
/// @return None
void SYSTEM_Initialize(void);

#endif /* CONFIG_H */