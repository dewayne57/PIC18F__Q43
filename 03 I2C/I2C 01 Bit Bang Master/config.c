/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for I2C 01 Bit Bang Master demonstration.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-11
 * 
 *   Configure Port B pins for use by UART 1 as follows: 
 *   RB0 - TX1 (output)
 *   RB1 - RX1 (input)
 *   RB2 - External IOC interrupt input
 * 
 *   Configure Port C pins for I2C bit bang master as follows:
 *   RC3 - I2C SCK (Serial Clock) - open-drain output
 *   RC4 - I2C SDA (Serial Data) - open-drain output
 * 
 *   The demonstration will use the internal 64MHz high frequency oscillator as the system clock.
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "../../Libraries/PPSLIB/pps.h"
#include "extern_ioc.h"

/// @brief Initialize system-level hardware used by the UART demonstration.
/// @param None
/// @return None
void SYSTEM_Initialize(void)
{
    /* Disable global interrupts while modifying shared hardware registers to prevent
       an ISR from running on partially-configured hardware. */
    INTCON0bits.GIEH = 0;
    INTCON0bits.GIEL = 0;
    INTCON0bits.IPEN = 1; // Enable priority interrupts, so we can use low-priority for the UART ISRs.

    /* The ANSEL (Analog SELect) registers control whether each I/O pin acts as an analog
       input (ANSEL bit = 1) or a digital I/O (ANSEL bit = 0).  Clearing them ensures that
       the UART TX/RX pins on Port B are in digital mode so the UART module can drive them.
       Ports C and D are cleared for the same reason - no analog inputs are used here. */
    ANSELB = 0x00;  // All Port B pins: digital mode
    ANSELC = 0x00;  // All Port C pins: digital mode
    ANSELD = 0x00;  // All Port D pins: digital mode

    /*
     * Enable all peripheral modules that are required
     */
    PMD0bits.SYSCMD = 0; // System clock network enabled
    PMD6bits.I2C1MD = 0; // I2C1 module enabled
    PMD6bits.U1MD = 0;   // UART 1 enabled

    // UART1 pin setup: RB0 = TX1, RB1 = RX1
    TRISBbits.TRISB0 = 0;
    TRISBbits.TRISB1 = 1;
    ANSELBbits.ANSELB0 = 0;
    ANSELBbits.ANSELB1 = 0;

    // I2C bit bang pin setup: RC3 = SCK, RC4 = SDA (configured as open-drain outputs)
    TRISCbits.TRISC3 = 1;  // RC3 initially high-impedance
    TRISCbits.TRISC4 = 1;  // RC4 initially high-impedance
    ANSELCbits.ANSELC3 = 0;
    ANSELCbits.ANSELC4 = 0;

    // External IOC on RB2 for interrupt input
    TRISBbits.TRISB2 = 1;   // RB2 is input
    ANSELBbits.ANSELB2 = 0; // RB2 is digital
    WPUBbits.WPUB2 = 1;     // Weak pull-up enabled on RB2

    PPS_Unlock();
    RB0PPS = 0x20;
    U1RXPPS = 0x09;
    PPS_Lock();

    __delay_ms(100); // Short delay to allow hardware to stabilize before enabling interrupts.

    /* Re-enable interrupts now that hardware registers are stable. */
    INTCON0bits.GIEH = 1;
    INTCON0bits.GIEL = 1;
}

