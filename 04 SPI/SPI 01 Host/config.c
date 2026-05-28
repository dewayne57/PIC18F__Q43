/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for SPI 01 Host demonstration.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-19
 *
 *   Configure Port B pins for use by UART 1 as follows:
 *   RB0 - TX1 (output)
 *   RB1 - RX1 (input)
 *   RB2 - External INT1 interrupt input
 *
 *   Configure Port C pins for SPI1 hardware module as follows:
 *   RC0..RC2 - SPI Device Address (decoded by 74LS138).
 *   RC3 - SPI1 SCLK (clock) - push-pull, driven by SPI1 module
 *   RC4 - SPI1 SDI (data in) - input, driven by SPI1 module
 *   RC5 - SPI1 SDO (data out) - push-pull, driven by SPI1 module
 *   RC6 - SPI1 SS (slave select) - push-pull, driven by SPI1 module
 *
 *   Configure Port D pins as active-low diagnostic LEDs:
 *   RD0..RD7 - startup, launch, ISR activity, and error indicators
 *
 *   The demonstration will use the internal 64MHz high frequency oscillator as the
 *   system clock.
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "../../Libraries/PPSLIB/pps.h"

/// @brief Initialize system-level hardware used by the SPI module host demonstration.
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
    ANSELB = 0x00; // All Port B pins: digital mode
    ANSELC = 0x00; // All Port C pins: digital mode
    ANSELD = 0x00; // All Port D pins: digital mode

    /*
     * Enable peripheral modules that are required.
     */
    PMD0bits.SYSCMD = 0; // System clock network enabled
    PMD0bits.CLKRMD = 0; // Clock Reference enabled
    PMD6bits.SPI1MD = 0; // SPI1 module enabled
    PMD6bits.U1MD = 0;   // UART1 enabled
    __delay_ms(10);      // Short delay to allow modules to stabilize after power-up

    // Port D diagnostic LEDs are active-low outputs.
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;
    TRISDbits.TRISD2 = 0;
    TRISDbits.TRISD3 = 0;
    LATDbits.LATD0 = 1;
    LATDbits.LATD1 = 1;
    LATDbits.LATD2 = 1;
    LATDbits.LATD3 = 1;

    // External INT1 on RB2 for MCP23S17 interrupt input
    TRISBbits.TRISB2 = 1;   // RB2 is input
    ANSELBbits.ANSELB2 = 0; // RB2 is digital
    WPUBbits.WPUB2 = 0;     // Weak pull-up disabled on RB2

    PPS_Unlock();
    INT1PPS = 0x0A;        // Map INT1 to RB2
    PPS_Lock();

    INTCON0bits.INT1EDG = 1; // Rising edge triggers INT1
    PIR6bits.INT1IF = 0;     // Clear any pending INT1 flag
    PIE6bits.INT1IE = 0;     // Enable INT1 after MCP23S17 is initialized

    /* Re-enable interrupts now that hardware registers are stable. */
    INTCON0bits.GIEH = 1;
    INTCON0bits.GIEL = 1;
}
