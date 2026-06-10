/* *****************************************************************************************
 *   File Name: config.h
 *   Description: Configuration header for the demonstration project.
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
#pragma config BOREN = 0              // Brown-out reset disabled for bring-up stability testing
#pragma config LPBOREN = OFF          // Low power brown-out reset is disabled
#pragma config IVT1WAY = 0            // IVTLOCK Set/cleared repeatedly
#pragma config MVECEN = 1             // Vectored interrupts enabled 
#pragma config PWRTS = 2              // Power up timer at 64mS
#pragma config MCLRE = 1              // Master clear retains that function
#pragma config XINST = OFF            // No extended instruction set
#pragma config LVP = 1                // Low voltage programming is enabled
#pragma config STVREN = ON            // Stack over/under flow causes reset
#pragma config PPS1WAY = 0            // PPSLOCK set/reset repeatedly
#pragma config ZCD = 1                // Zero-cross detection is disabled
#pragma config BORV = 0               // Brown-out voltage selection (unused while BOREN is disabled)
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

#define CRLF "\r\n"

void SYSTEM_Initialize(void);

// Operating voltage for the demonstration project (in mV). This is used as the
// reference voltage for converting ADC counts to engineering units.
#define ADCC_VREF_POSITIVE_MV 5000UL

// The maximum count value for a 12-bit ADC is 4095 (2^12 - 1).
#define ADCC_MAX_COUNTS 4095UL

// Convert between mV and 12-bit ADCC counts with rounding.
#define ADCC_MV_TO_COUNTS(mv) ((((unsigned long)(mv) * ADCC_MAX_COUNTS) + (ADCC_VREF_POSITIVE_MV / 2UL)) / ADCC_VREF_POSITIVE_MV)
#define ADCC_COUNTS_TO_MV(counts) ((((unsigned long)(counts) * ADCC_VREF_POSITIVE_MV) + (ADCC_MAX_COUNTS / 2UL)) / ADCC_MAX_COUNTS)

// Number of samples the ADCC hardware accumulates and averages per result.
// Must be a power of 2 between 2 and 256.
#define ADCC_OVERSAMPLE_COUNT  16U

// ADCON2.CRS = log2(ADCC_OVERSAMPLE_COUNT).  Right-shifts the accumulator so
// that ADFLTR holds a 12-bit average (same resolution as a single conversion).
#define ADCC_OVERSAMPLE_CRS    4U

// ADRPT = ADCC_OVERSAMPLE_COUNT - 1.  The ADCC performs ADRPT+1 conversions
// per average group.
#define ADCC_OVERSAMPLE_ADRPT  (ADCC_OVERSAMPLE_COUNT - 1U)

// IIR filter coefficient applied in firmware after each hardware average:
//   filtered = ((den - num) * old + num * new) / den
#define ADCC_FILTER_ALPHA_NUM 1U
#define ADCC_FILTER_ALPHA_DEN 4U

// UART reporting cadence for the filtered value.
#define ADCC_REPORT_PERIOD_MS 100U

#endif /* CONFIG_H */