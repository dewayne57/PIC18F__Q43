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

// Number of samples to average for oversampling (must be a power of 2)
#define ADCC_OVERSAMPLE_COUNT 16 

// The datasheet specifies that the internal temperature sensor has a gain of 2.048 mV/°C in the
// high temperature range, which corresponds to a fixed voltage reference setting of 2.048V for the 
// ADC. However, due to manufacturing variations, the actual gain may differ slightly from this 
// nominal value. When the chip was manufactured, the factory measured the actual gain of the 
// internal temperature sensor in the high temperature range and stored it in program memory. This
// factory-calibrated gain value can then be used to improve the accuracy of temperature readings.

// The address of a 16-bit calibration value stored in program memory that represents the gain 
// for the high temperature range of the internal temperature sensor. This value is device-specific 
// and must be determined through calibration.
#define DIA_HIGH_RANGE_GAIN  2C002Ah

// The address of a 16-bit calibration value stored in program memory that represents the offset
// for the high temperature range of the internal temperature sensor. This value is device-specific
// and must be determined through calibration.
#define DIA_HIGH_RANGE_OFFSET 2C002Eh

// The data sheet recommends at least 10 samples averaged of the internal temperature sensor for 
// stable readings.  It also suggested calculating the temperature by applying the gain and
// offset calibration values to the raw ADC reading for each sample, then averaging the calculated 
// temperatures.  However, this approach requires more processing power and memory.  Instead, this
// project will simply average the raw ADC readings and then apply the gain and offset calibration 
// values once.  The calculated value will be the same, and it means we can use the ADCC module's 
// built-in oversampling and averaging features to handle the averaging of raw ADC readings, 
// which is more efficient.
// 
// Therefore, the actual formula used in this project to calculate the temperature from the averaged 
// raw ADC reading is:
//
//    Temperature (°C) = (ADCCAverage * gain) / 256 + offset
//
// where the Factory Gain Calibration and Factory Offset Calibration values are read from the 
// specified program memory addresses.  Note, all calculations are done in fixed-point math
// using signed variables (+ and -) to avoid the overhead of floating point operations.

#define FVR_FIXED_VOLTAGE_2_048V 0b10
void SYSTEM_Initialize(void);

#endif /* CONFIG_H */