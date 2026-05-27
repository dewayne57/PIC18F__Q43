/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for I2C 02 Module Host demonstration.
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

    /* Clear ANSEL registers so all used pins are in digital mode. */
    ANSELB = 0x00;  // All Port B pins: digital mode
    ANSELC = 0x00;  // All Port C pins: digital mode
    ANSELD = 0x00;  // All Port D pins: digital mode

    /*
     * Enable peripheral modules that are required.
     */
    PMD0bits.SYSCMD = 0; // System clock network enabled
    PMD0bits.CLKRMD = 0;   // Clock Reference enabled
    PMD6bits.I2C1MD = 0; // I2C1 module enabled
    PMD6bits.U1MD   = 0; // UART1 enabled
    __delay_ms(10); // Short delay to allow modules to stabilize after power-up

    // Port D diagnostic LEDs are active-low outputs.
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;
    TRISDbits.TRISD2 = 0;
    TRISDbits.TRISD3 = 0;
    LATDbits.LATD0 = 1;
    LATDbits.LATD1 = 1;
    LATDbits.LATD2 = 1;
    LATDbits.LATD3 = 1;

    // External INT1 on RB2 for MCP23017 interrupt input
    TRISBbits.TRISB2 = 1;   // RB2 is input
    ANSELBbits.ANSELB2 = 0; // RB2 is digital
    WPUBbits.WPUB2 = 0;     // Weak pull-up disabled on RB2

    PPS_Unlock();
    // INT1 input <- RB2 (port B=0b01, pin2=0b010 -> 0x0A)
    INT1PPS = 0x0A;
    PPS_Lock();

    INTCON0bits.INT1EDG = 1; // Rising edge triggers INT1
    PIR6bits.INT1IF = 0;     // Clear any pending INT1 flag
    PIE6bits.INT1IE = 0;     // Enable INT1 after MCP23017 is initialized

    /* Re-enable interrupts now that hardware registers are stable. */
    INTCON0bits.GIEH = 1;
    INTCON0bits.GIEL = 1;
}

