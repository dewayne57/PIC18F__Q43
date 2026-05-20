/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for I2C 02 Module Master demonstration.
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

/// @brief Initialize system-level hardware used by the I2C module master demonstration.
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
    PMD6bits.I2C1MD = 0; // I2C1 module enabled
    PMD6bits.U1MD   = 0; // UART1 enabled

    // I2C1 pin setup: RC3 = SCL, RC4 = SDA (open-drain with 2x internal pull-up)
    TRISCbits.TRISC3 = 1;   // RC3 as input (released high at idle)
    TRISCbits.TRISC4 = 1;   // RC4 as input (released high at idle)
    ANSELCbits.ANSELC3 = 0; // RC3 digital
    ANSELCbits.ANSELC4 = 0; // RC4 digital
    ODCONCbits.ODCC3 = 1;   // RC3 open-drain
    ODCONCbits.ODCC4 = 1;   // RC4 open-drain
    RC3I2Cbits.I2CPU = 0b10; // RC3 (SCL): 2x internal I2C pull-up (no external resistor needed)
    RC4I2Cbits.I2CPU = 0b10; // RC4 (SDA): 2x internal I2C pull-up (no external resistor needed)

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
    // Route I2C1 SCL output to RC3 (PPS output code 0x37 = I2C1SCL)
    RC3PPS = 0x37;
    // Route I2C1 SDA output to RC4 (PPS output code 0x38 = I2C1SDA)
    RC4PPS = 0x38;
    // Map I2C1 SCL input from RC3 (RC3 PPS address: port C=0b10, pin3=0b011 -> 0x13)
    I2C1SCLPPS = 0x13;
    // Map I2C1 SDA input from RC4 (RC4 PPS address: port C=0b10, pin4=0b100 -> 0x14)
    I2C1SDAPPS = 0x14;
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

