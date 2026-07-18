/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System configuration and initialization for the demonstration project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-19
 * 
 *   Configure Port B pins for use by UART 1 as follows:
 *   RB0 - TX1 (output)
 *   RB1 - RX1 (input)
 *   RB2 - External INT1 interrupt input
 * 
 *   Configure Port C pins for I2C1 hardware module as follows:
 *   RC3 - I2C1 SCL (clock) - open-drain, 2x internal pull-up via RC3I2C
 *   RC4 - I2C1 SDA (data)  - open-drain, 2x internal pull-up via RC4I2C
 *
 *   Configure Port D pins as active-low diagnostic LEDs:
 *   RD0..RD3 - startup, launch, ISR activity, and error indicators
 * 
 *   The demonstration will use the internal 64MHz high frequency oscillator as the system clock.
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "../../Libraries/PPSLIB/pps.h"

/// @brief Initialize system-level hardware used by the I2C module host demonstration.
/// @param None
/// @return None
void SYSTEM_Initialize(void)
{
    /* Disable global interrupts while modifying shared hardware registers to prevent
       an ISR from running on partially-configured hardware. */
    INTCON0bits.GIEH = 0;
    INTCON0bits.GIEL = 0;
    INTCON0bits.IPEN = 1; // Enable priority interrupts for low-priority UART ISRs.

    /* Set up the oscillators as needed */
    OSCFRQbits.HFFRQ = 0b1000; // HFINTOSC is 64MHz
    OSCENbits.HFOEN = 1;    // Enable HFINTOSC 
    OSCENbits.MFOEN = 1;    // Enable MFINTOSC
    OSCENbits.LFOEN = 1;    // Enable LFINTOSC
    ACTCONbits.ACTEN = 1; // Enable active clock tuning
    ACTCONbits.ACTUD = 0;   // Hardware tuning
    while (!OSCSTATbits.HFOR && !OSCSTATbits.MFOR && !OSCSTATbits.LFOR)
        ; // Wait for all oscillators to stabilize        
    
    /* Clear ANSEL registers so all used pins are in digital mode. */
    ANSELB = 0x00;  // All Port B pins: digital mode
    ANSELC = 0x00;  // All Port C pins: digital mode
    ANSELD = 0x00;  // All Port D pins: digital mode

    WPUA = 0x00;  // All Port A pins: weak pull-ups disabled
    WPUB = 0x00;  // All Port B pins: weak pull-ups
    WPUC = 0x00;  // All Port C pins: weak pull-ups disabled
    WPUD = 0x00;  // All Port D pins: weak pull-ups
    WPUE = 0x00;  // All Port E pins: weak pull-ups disabled

    /*
     * Enable peripheral modules that are required.
     */
    PMD0bits.SYSCMD = 0; // System clock network enabled
    PMD0bits.CLKRMD = 0;   // Clock Reference enabled
    PMD6bits.I2C1MD = 0; // I2C1 module enabled
    PMD6bits.U1MD   = 0; // UART1 enabled
    __delay_ms(10); // Short delay to allow modules to stabilize after power-up

    // External INT1 on RB2 for MCP23017 interrupt input
    TRISBbits.TRISB2 = 1;   // RB2 is external interrupt
    WPUBbits.WPUB2 = 1;     // External interrupt is pulled up
    TRISBbits.TRISB3 = 0;   // RB3 is output (reset for MCP23017)
    
    PPS_Unlock();
    // INT1 input <- RB2 
    INT1PPS = 0x0A;
    PPS_Lock();

    INTCON0bits.INT1EDG = 0; // Falling edge triggers INT1 (MCP23017 INT active-low)
    PIR6bits.INT1IF = 0;     // Clear any pending INT1 flag
    PIE6bits.INT1IE = 0;     // Enable INT1 after MCP23017 is initialized

    /* Re-enable interrupts now that hardware registers are stable. */
    INTCON0bits.GIEH = 1;
    INTCON0bits.GIEL = 1;
}

