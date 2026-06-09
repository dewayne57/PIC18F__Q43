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

// Define the ADC window thresholds (in mV) for the demonstration project.  The potentiometer
// voltage is measured by the ADC and compared to these thresholds to determine which window 
// status LED to light. The setpoint is the midpoint of the window, and the tolerance defines
// how wide the window is around the setpoint.  For example, with a setpoint of 2500mV and a
// tolerance of 500mV: The window is from 2000mV to 3000mV.  If the potentiometer voltage is 
// below 2000mV, the "LOW" LED lights.  If the potentiometer voltage is above 3000mV, the 
// "HIGH" LED lights.  If the potentiometer voltage is within the window, the "IN WINDOW" 
// LED lights.

// Operating voltage for the demonstration project (in mV).  This is used as the reference 
// voltage for the ADC.  The ADC converts the input voltage to a digital count based on this 
// reference.  For example, with a 5V reference and a 12-bit ADC, the count value for a given 
// input voltage can be calculated as (input_voltage / reference_voltage) * (2^12 - 1).
#define ADCC_VREF_POSITIVE_MV 5000UL

// The maximum count value for a 12-bit ADC is 4095 (2^12 - 1). This is used to convert 
// between millivolts and ADC counts.
#define ADCC_MAX_COUNTS 4095UL

// Define the window setpoint and tolerance (in mV) for the demonstration project. The 
// setpoint is the target voltage that the potentiometer should be at, and the tolerance 
// defines how much above or below the setpoint is still considered "in window". For 
// example, with a setpoint of 2500mV and a tolerance of 500mV, the window would be from 
// 2000mV to 3000mV. If the potentiometer voltage is below 2000mV, it would be considered 
// "LOW". If it's above 3000mV, it would be considered "HIGH". If it's between 2000mV and 
// 3000mV, it would be considered "IN WINDOW".
#define WINDOW_SETPOINT_MV 2500UL 
#define WINDOW_TOLERANCE_MV 500UL 

// The following macros are used to perform the necessary calculations to convert the 
// window setpoint and thresholds from millivolts to 12-bit ADCC counts. The
// ADCC_MV_TO_COUNTS macro takes a voltage in millivolts and converts it to the
// corresponding ADC count value based on the defined reference voltage and maximum count.
// The WINDOW_LOW_MV and WINDOW_HIGH_MV macros calculate the low and high threshold voltages
// based on the setpoint and tolerance, ensuring that they do not go below 0mV or above the
// reference voltage. Finally, the WINDOW_SETPOINT_COUNTS, WINDOW_LOW_COUNTS, and
// WINDOW_HIGH_COUNTS macros convert the setpoint and threshold voltages to their respective
// ADC count values for use in the ADC configuration.

/* Convert millivolts to 12-bit ADCC counts with rounding for a 0..VREF range. */
#define ADCC_MV_TO_COUNTS(mv) ((((unsigned long)(mv) * ADCC_MAX_COUNTS) + (ADCC_VREF_POSITIVE_MV / 2UL)) / ADCC_VREF_POSITIVE_MV)

#define WINDOW_LOW_MV ((WINDOW_SETPOINT_MV > WINDOW_TOLERANCE_MV) ? (WINDOW_SETPOINT_MV - WINDOW_TOLERANCE_MV) : 0UL)
#define WINDOW_HIGH_MV (((WINDOW_SETPOINT_MV + WINDOW_TOLERANCE_MV) > ADCC_VREF_POSITIVE_MV) ? ADCC_VREF_POSITIVE_MV : (WINDOW_SETPOINT_MV + WINDOW_TOLERANCE_MV))

#define WINDOW_SETPOINT_COUNTS ADCC_MV_TO_COUNTS(WINDOW_SETPOINT_MV)
#define WINDOW_LOW_COUNTS ADCC_MV_TO_COUNTS(WINDOW_LOW_MV)
#define WINDOW_HIGH_COUNTS ADCC_MV_TO_COUNTS(WINDOW_HIGH_MV)

#endif /* CONFIG_H */