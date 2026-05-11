/* *****************************************************************************************
 *   File Name: config.h
 *   Description: This file contains the configuration settings for the IOC Single project.
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
#pragma config FEXTOSC = OFF           // Dont use the external oscillator
#pragma config RSTOSC = HFINTOSC_64MHZ // Use internal 64MHz high frequency osc
#pragma config CSWEN = OFF             // No clock switching allowed
#pragma config FCMEN = OFF             // Fail-safe clock monitor is disabled
#pragma config PR1WAY = 0              // PRLOCKED Set/Cleared repeatedly
#pragma config CLKOUTEN = 0            // Clock out is enabled on RA6
#pragma config BOREN = 3               // Brown-out reset is enabled
#pragma config LPBOREN = OFF           // Low power brown-out reset is disabled
#pragma config IVT1WAY = 0             // IVTLOCK Set/cleared repeatedly
#pragma config MVECEN = 0              // Vectored interrupts disabled <== Note: This is required
                                       // for single level interrupts
#pragma config PWRTS = 2               // Power up timer at 64mS
#pragma config MCLRE = 1               // Master clear retains that function
#pragma config XINST = OFF             // No extended instruction set
#pragma config LVP = 1                 // Low voltage programming is enabled
#pragma config STVREN = ON             // Stack over/under flow causes reset
#pragma config PPS1WAY = 0             // PPSLOCK set/reset repeatedly
#pragma config ZCD = 1                 // Zero-cross detection is disabled
#pragma config BORV = 0                // Brown-out voltage is set to 2.85V
#pragma config WDTE = OFF              // No watch dog timer
#pragma config SAFEN = OFF             // Storage area flash is disabled
#pragma config BBEN = OFF              // Boot block is disabled
#pragma config WRTAPP = OFF            // Application block is NOT write protected
#pragma config WRTSAF = OFF            // SAF area is not write protected
#pragma config WRTC = OFF              // Configuration registers are NOT write protected
#pragma config WRTB = OFF              // Boot block is not write protected
#pragma config WRTD = OFF              // Data EEPROM is not write protected
#pragma config CP = OFF                // Code is not protected

#define _XTAL_FREQ 64000000UL          // Define the system clock frequency for delay functions

void SYSTEM_Initialize(void);

#endif /* CONFIG_H */