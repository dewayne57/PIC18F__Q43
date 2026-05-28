/* *****************************************************************************************
 *   File Name: config.h
 *   Description: This file contains the configuration settings for the demonstration project.
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
#pragma config MVECEN = 1             // Vectored interrupts enabled
#pragma config PWRTS = 2              // Power up timer at 64mS
#pragma config MCLRE = 1              // Host clear retains that function
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
/*
 * Define the MCP23S17 I2C address.
 *
 * This value is used on the SPI address lines (RC0..RC2) to select which MCP23S17 
 * device to communicate with.  The 74LS138 decoder on the SPI slave board will decode 
 * these 3 bits to select one of the connected MCP23S17 devices.  The SPI Slave Select 
 * line (RC6) must also be driven low to select the SPI slave device before communicating 
 * with it (this enables the 74LS138 decoder).  By using the SS line in conjunction with the
 * address lines, the demonstration can support up to 8 MCP23S17 devices on the same SPI bus
 * and also allows the address lines to be used for other purposes if desired (e.g. 
 * controlling other SPI devices on the same bus that use a different slave select scheme).
 * The Q43 family offers multiple SPI modules, so additional MCP23S17 devices could be 
 * connected to other SPI modules.  This offers a lot of flexibility for expanding the 
 * demonstration to include more devices or different configurations.
 */
#define MCP23S17_ADDR 0x00

/// @brief Initialize the system, including clock settings, pin configurations for SPI and
/// external IOC, and any other necessary hardware setup for the SPI module host
/// demonstration project.  This function should be called at the beginning of the main
/// function to ensure that all hardware is properly initialized before use.
/// @param  None
/// @return None
void SYSTEM_Initialize(void);

#endif /* CONFIG_H */