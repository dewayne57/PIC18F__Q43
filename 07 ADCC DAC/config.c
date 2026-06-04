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
   ANSELA = 0xFF;        // All port A pins are analog
   TRISAbits.TRISA0 = 1; // RA0 as input for the potentiometer voltage measurement
   ANSELB = 0x00;        // All Port B pins: digital mode
   ANSELC = 0x00;        // All Port C pins: digital mode
   ANSELD = 0x00;        // All Port D pins: digital mode

   /* RB0 and RB1 are reserved for the UART module's use.  RB2..RB4 are window status
      LEDs (active LOW): RB2=LOW, RB3=HIGH, RB4=IN WINDOW. */
   TRISBbits.TRISB2 = 0;
   TRISBbits.TRISB3 = 0;
   TRISBbits.TRISB4 = 0;
   LATBbits.LATB2 = 1;
   LATBbits.LATB3 = 1;
   LATBbits.LATB4 = 1;

   /* Initialize the ADCC for continuous Window Comparator use with the low and high
      thresholds defined in config.h */
   ADCON0bits.ADON = 0; // Ensure the ADC is off before configuring
   ADCON0bits.CONT = 1; // Enable continuous conversion mode for the demonstration project
   ADCON0bits.CS = 0;   // ADC clock derived from system clock (ADCCLK = FOSC)
   ADCON0bits.FM = 1;   // Right justified result format (full 12-bit resolution)
   ADCON0bits.GO = 0;   // Clear the GO bit to start with a known state

   ADCON1bits.PPOL = 0; // Unipolar input
   ADCON1bits.IPEN = 0; // Use precharge defined by PPOL and GPOL
   ADCON1bits.GPOL = 0; // Unipolar input
   ADCON1bits.DSEN = 0; // No double sampling

   ADCON2bits.PSIS = 0;   // ADRES is transferred to ADPREV at start-of-conversion
   ADCON2bits.CRS = 0;    // ADC Accumulated Calculation Right Shift is not used
   ADCON2bits.MD = 0b000; // Basic mode (raw conversion result in ADRES)

   ADCON3bits.CALC = 0b000; // No post processing calculation on the ADC result
   ADCON3bits.TMD = 0b000;  // Interrupt at end of each conversion in continuous mode

   ADCLK = 32; // ADC clock derived from system clock divided by 64 (1us TAD)

   ADREFbits.NREF = 0;    // Use AVSS as ADC negative reference
   ADREFbits.PREF = 0b00; // Use VDD as ADC positive reference

   ADPCHbits.PCH = 0b00000; // Select AN0 (RA0) as the ADC input channel
   // The acquisition time minimum value is computed from a formula in the datasheet
   // that takes into account the ADC's internal sample-and-hold capacitor and the
   // impedance of the signal source.  For a potentiometer connected to VDD and GND,
   // the maximum source impedance is Rpot/2.  Assuming a 10k potentiometer, that gives
   // a maximum source impedance of 5k.  The formula for acquisition time is
   // TAD * (ADPRE + ADACQ) >= Source Impedance * Sample-and-Hold Capacitance.
   // The sample-and-hold capacitance is typically around 5pF, so we can rearrange the
   // formula to find suitable values for ADPRE and ADACQ:
   // ADPRE + ADACQ >= (Source Impedance * Sample-and-Hold Capacitance) / TAD.
   // Plugging in the numbers gives us ADPRE + ADACQ >= (5000 * 5e-12) / 1e-6 = 25.
   // We can choose ADPRE = 15 and ADACQ = 15 to satisfy this requirement while
   // keeping acquisition time reasonable.
   ADPRE = 15;  // 15 clock ticks for pre-charge (15us at 1us TAD)
   ADACQ = 15;  // 15 clock ticks for acquisition time (15us at 1us TAD)
   ADCAP = 0;   // No additional Sample/Hold capacitance
   ADCRPT = 0;  // Continuous mode does not use the repeat counter, so set it to 0
   ADCNT = 0;   // Not used in legacy mode
   ADFLTRH = 0; // No digital filtering
   ADFLTRL = 0; // No digital filtering

   ADSTPT = WINDOW_SETPOINT_COUNTS; // Setpoint converted from mV to 12-bit ADCC counts
   ADLTH = WINDOW_LOW_COUNTS;       // Lower threshold converted from mV to counts
   ADUTH = WINDOW_HIGH_COUNTS;      // Upper threshold converted from mV to counts

   ADACT = 0; // No auto-conversion trigger source
   ADCP = 0;  // A/D Charge Pump is disabled. This is needed only if the operating
              // voltage is below 5v.

   PIE1bits.ADIE = 1; // Enable ADC conversion complete interrupt
   PIE2bits.ADTIE = 0;
   PIR1bits.ADIF = 0; // Clear ADC interrupt flag before enabling interrupts globally
   PIR2bits.ADTIF = 0;

   ADCON0bits.ADON = 1; // Turn on the ADC after configuration is complete
   ADCON0bits.GO = 1;   // Start continuous conversions

   /* Re-enable interrupts now that hardware registers are stable. */
   INTCON0bits.GIEL = 1; /* Enable low priority interrupts (required for low_priority vectored ISRs). */
   INTCON0bits.GIEH = 1; /* Enable high priority interrupts (master enable). */
}
