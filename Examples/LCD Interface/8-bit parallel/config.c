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
   ANSELE = 0x00; // All Port E pins: digital mode

   TRISA = 0xFF; // All port A pins are inputs (analog)
   TRISB = 0x00; // All Port B pins: outputs (digital) (UARTLIB will ovberride as needed for TX/RX)
   TRISC = 0xFF; // All Port C pins: inputs (digital) (unused in this project, but set as inputs 
                 // to avoid driving them)
   TRISD = 0x00; // All Port D pins: outputs (digital)
   TRISE = 0x00; // All pins: output mode (no rising edge interrupts)

   /* Re-enable interrupts now that hardware registers are stable. */
   INTCON0bits.GIEL = 1; /* Enable low priority interrupts (required for low_priority vectored ISRs). */
   INTCON0bits.GIEH = 1; /* Enable high priority interrupts (master enable). */
}
