/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for the demonstration project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
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
   ANSELB = 0x00; // All Port B pins: digital mode
   ANSELC = 0x00; // All Port C pins: digital mode
   ANSELD = 0x00; // All Port D pins: digital mode

   /* RB2 drives an active-low status LED for FVR state indication.
      Drive high initially so the LED is off until FVR is enabled. */
   TRISBbits.TRISB2 = 0;
   LATBbits.LATB2 = 1;

   /* Re-enable interrupts now that hardware registers are stable. */
   INTCON0bits.GIEL = 1; /* Enable low priority interrupts (required for low_priority vectored ISRs). */
   INTCON0bits.GIEH = 1; /* Enable high priority interrupts (master enable). */
}
