/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for the demonstration project.
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
 *
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"

/// @brief Initialize system-level hardware used by the demonstration project.
/// @param None
/// @return None
void SYSTEM_Initialize(void)
{
   /* Disable global interrupts while modifying shared hardware registers to prevent
   an ISR from running on partially-configured hardware. */
   INTCON0bits.GIEH = 0;
   INTCON0bits.GIEL = 0;

   /* The ANSEL (Analog SELect) registers control whether each I/O pin acts as an analog
      input (ANSEL bit = 1) or a digital I/O (ANSEL bit = 0).  Clearing them ensures that
      the UART TX/RX pins on Port B are in digital mode so the UART module can drive them.
      Ports C and D are cleared for the same reason - no analog inputs are used here. */
   ANSELA = 0xFF; // All port A pins are analog
   ANSELB = 0x00; // All Port B pins: digital mode
   ANSELC = 0x00; // All Port C pins: digital mode
   ANSELD = 0x00; // All Port D pins: digital mode

   FVRCON = 0x00;                                // Reset FVR control register to default state (disabled)
   FVRCONbits.TSEN = 1;                          // Enable the temperature sensor
   FVRCONbits.TSRNG = 1;                         // High temperature sensor range (-40 to +125C)
   FVRCONbits.ADFVR = FVR_FIXED_VOLTAGE_2_048V;  // ADC FVR default: 2.048V
   FVRCONbits.CDAFVR = FVR_FIXED_VOLTAGE_2_048V; // DAC/Comparator FVR default: 2.048V
   FVRCONbits.EN = 1;                            // Enable the FVR module
   while (!FVRCONbits.RDY)
   {
      // Wait for the ready bit to be set, indicating the FVR is now enabled and stable
   }

   // setup the ADCC module for continuous conversion with oversampling and averaging, with
   // the FVR as the reference voltage, and a conversion completion interrupt enabled.
   ADCCON0 = 0x00;       // Reset ADCCON0 to default
   ADCCON0bits.CONT = 1; // Continuous conversion mode
   ADCCON0bits.FM = 1;   // Right justified result format
   ADCCON0bits.CS = 0;   // Clock source = FOSC/64 (default)

   ADCCON1 = 0; // Reset ADCCON1 to default (no precharge or guard rings)

   ADCCON2 = 0;                                  // Reset ADCCON2 to default
   ADCCON2bits.CRS = ADCC_AVERAGE_DIVISOR_SHIFT; // Set the shift amount for averaging

   ADCCON3 = 0x00;             // Reset ADCON3 to default
   ADSTAT = 0x00;              // Reset ADSTAT to default
   ADCLK = ADCC_CLOCK_DIVISOR; // Set the ADC clock source divisor
   ADREFbits.ADNREF = 0;       // Set ADC negative reference to VSS
   ADREFbits.ADPREF = 0b11;    // Set ADC positive reference to FVR (2.048V)
   ADPCHbits.PCH = 0b111100;   // Set ADC positive channel to temperature sensor (channel 60)

   ADCRPT = ADCC_OVERSAMPLE_COUNT - 1; // Set oversample count for averaging
   ADCNT = 0;                          // Clear the ADC conversion count
   ADCCON2bits.MD = 0b010;             // Set ADC mode to continuous conversion with oversampling
                                       // and averaging
   ADPRE = ADCC_PRECHARGE_INTERVAL;    // Set precharge interval for ADC
   APACQ = ADCC_ACQUISITION_INTERVAL;  // Set acquisition interval for ADC
   ADCAP = 0;                          // No additional sample-and-hold capacitance
   ADACT = 0;                          // No external triggering for ADC conversion
   ADCP = 0;                           // No charge pump for ADC (operating at 5V)

   ADCCON0bits.ON = 1; // Enable the ADC module
   ADCCON0bits.GO = 1; // Start the first ADC conversion

   /* Re-enable interrupts now that hardware registers are stable. */
   INTCON0bits.GIEL = 1; /* Enable low priority interrupts (required for low_priority vectored ISRs). */
   INTCON0bits.GIEH = 1; /* Enable high priority interrupts (master enable). */
}
