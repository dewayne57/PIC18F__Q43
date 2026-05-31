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
 *   RC7 - I/O Extender Reset (active low)
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

    // Turn off all peripherals we are not using
    PMD0bits.SYSCMD = 0; // System clock network is enabled
    PMD0bits.FVRMD = 1;  // Fixed voltage reference module is disabled
    PMD0bits.HLVDMD = 1; // High/Low-Voltage Detect module is disabled
    PMD0bits.CRCMD = 1;  // CRC module is disabled
    PMD0bits.SCANMD = 1; // Scan module is disabled
    PMD0bits.CLKRMD = 0; // Clock Reference module is enabled
    PMD0bits.IOCMD = 0;  // Interrupt on change module is enabled

    PMD1bits.SMT1MD = 1; // SMT1 module is disabled
    PMD1bits.TMR0MD = 1; // Timer0 module is disabled
    PMD1bits.TMR1MD = 1; // Timer1 module is disabled
    PMD1bits.TMR2MD = 1; // Timer2 module is disabled
    PMD1bits.TMR3MD = 1; // Timer3 module is disabled
    PMD1bits.TMR4MD = 1; // Timer4 module is disabled
    PMD1bits.TMR5MD = 1; // Timer5 module is disabled

    PMD3bits.ACTMD = 1;  // Active clock tuning module is disabled
    PMD3bits.DAC1MD = 1; // DAC1 module is disabled
    PMD3bits.ADCMD = 1;  // ADC module is disabled
    PMD3bits.CM1MD = 1;  // Comparator1 module is disabled
    PMD3bits.CM2MD = 1;  // Comparator2 module is disabled
    PMD3bits.ZCDMD = 1;  // Zero-cross detect module is disabled

    PMD4bits.CWG3MD = 1; // Complimentary waveform generator 3 disabled
    PMD4bits.CWG2MD = 1; // Complimentary waveform generator 2 disabled
    PMD4bits.CWG1MD = 1; // Complimentary waveform generator 1 disabled
    PMD4bits.DSM1MD = 1; // Digital Signal Modulator disabled
    PMD4bits.NCO3MD = 1; // Numerically Controlled Oscillator 3 disabled
    PMD4bits.NCO2MD = 1; // Numerically Controlled Oscillator 2 disabled
    PMD4bits.NCO1MD = 1; // Numerically Controlled Oscillator 1 disabled

    PMD5bits.PWM3MD = 1; // PWM module 3 disabled
    PMD5bits.PWM2MD = 1; // PWM module 2 disabled
    PMD5bits.PWM1MD = 1; // PWM module 1 disabled
    PMD5bits.CCP3MD = 1; // CCP module 3 disabled
    PMD5bits.CCP2MD = 1; // CCP module 2 disabled
    PMD5bits.CCP1MD = 1; // CCP module 1 disabled

    PMD6bits.U5MD = 1;   // UART5 module is disabled
    PMD6bits.U4MD = 1;   // UART4 module is disabled
    PMD6bits.U3MD = 1;   // UART3 module is disabled
    PMD6bits.U2MD = 1;   // UART2 module is disabled
    PMD6bits.U1MD = 0;   // UART1 module is enabled
    PMD6bits.SPI2MD = 1; // SPI2 module is disabled
    PMD6bits.SPI1MD = 0; // SPI1 module is enabled
    PMD6bits.I2C1MD = 1; // I2C1 module is disabled
    PMD7 = 0xFF; // All modules in PMD7 are disabled (CLC)
    PMD8 = 0xFF; // All modules in PMD8 are disabled (DMA)

    /* Clear ANSEL registers so all used pins are in digital mode. */
    ANSELB = 0x00; // All Port B pins: digital mode
    ANSELC = 0x00; // All Port C pins: digital mode
    ANSELD = 0x00; // All Port D pins: digital mode

    // External INT1 on RB2 for MCP23S17 interrupt input
    TRISBbits.TRISB2 = 1;   // RB2 is input
    ANSELBbits.ANSELB2 = 0; // RB2 is digital
    WPUBbits.WPUB2 = 0;     // Weak pull-up disabled on RB2

    // Now, set up port C rc0..2 as the address output lines to select the appropriate
    // SPI device.  These are simple digital push-pull output pins.  Set them initially
    // to all ones.
    TRISCbits.TRISC0 = 0; // Set RC0 as output
    TRISCbits.TRISC1 = 0; // Set RC1 as output
    TRISCbits.TRISC2 = 0; // Set RC2 as output
    TRISCbits.TRISC7 = 0; // Set RC7 as output
    LATCbits.LATC0 = 1;   // Set RC0 high
    LATCbits.LATC1 = 1;   // Set RC1 high
    LATCbits.LATC2 = 1;   // Set RC2 high
    LATCbits.LATC7 = 0;   // Hold i/o extender in reset
    ODCONCbits.ODCC0 = 0; // Set RC0 as push-pull
    ODCONCbits.ODCC1 = 0; // Set RC1 as push-pull
    ODCONCbits.ODCC2 = 0; // Set RC2 as push-pull
    ODCONCbits.ODCC7 = 0; // Set RC7 as push-pull

    // Port D diagnostic LEDs are active-low outputs.
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;
    TRISDbits.TRISD2 = 0;
    TRISDbits.TRISD3 = 0;
    LATDbits.LATD0 = 1;
    LATDbits.LATD1 = 1;
    LATDbits.LATD2 = 1;
    LATDbits.LATD3 = 1;

    PPS_Unlock();
    INT1PPS = 0x0A; // Map INT1 to RB2
    PPS_Lock();

    INTCON0bits.INT1EDG = 1; // Rising edge triggers INT1
    PIR6bits.INT1IF = 0;     // Clear any pending INT1 flag
    PIE6bits.INT1IE = 0;     // Enable INT1 after MCP23S17 is initialized

    /* Re-enable interrupts now that hardware registers are stable. */
    INTCON0bits.GIEH = 1;
    INTCON0bits.GIEL = 1;
}
