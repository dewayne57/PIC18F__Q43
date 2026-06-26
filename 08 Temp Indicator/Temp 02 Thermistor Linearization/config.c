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
      input (ANSEL bit = 1) or a digital I/O (ANSEL bit = 0). */
   TRISA = 0xFF;  // All port A pins are inputs to disable the digital drivers
   ANSELA = 0xFF; // All port A pins are analog
   ANSELB = 0x00; // All Port B pins: digital mode
   ANSELC = 0x00; // All Port C pins: digital mode
   ANSELD = 0x00; // All Port D pins: digital mode

   FVRCON = 0x00;                                // Reset FVR control register to default state (disabled)
   FVRCONbits.ADFVR = FVR_FIXED_VOLTAGE_2_048V;  // ADC FVR default: 2.048V
   FVRCONbits.CDAFVR = FVR_FIXED_VOLTAGE_2_048V; // DAC/Comparator FVR default: 2.048V
   FVRCONbits.EN = 1;                            // Enable the FVR module
   while (!FVRCONbits.RDY)
   {
      // Wait for the ready bit to be set, indicating the FVR is now enabled and stable
   }

   
   /* Re-enable interrupts now that hardware registers are stable. */
   INTCON0bits.GIEL = 1; /* Enable low priority interrupts (required for low_priority vectored ISRs). */
   INTCON0bits.GIEH = 1; /* Enable high priority interrupts (master enable). */
}
