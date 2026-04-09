/* *****************************************************************************************
 *   File Name: config.c
 *   Description: This file contains the configuration settings for the IOC Single project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-09
 ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include <stdbool.h>

static bool system_initialized = false;

/**
 * Initializes the system by configuring the I/O ports, disabling all peripheral modules, and
 * enabling global interrupts. This function should be called at the beginning of the main program
 * to set up the microcontroller for operation.
 */
void SYSTEM_Initialize(void)
{
    system_initialized = false;

    /*
     * Disable interrupts and force flat (single-vector) mode
     */
    INTCON0bits.GIE = 0;
    INTCON0bits.IPEN = 0;

    /*
     * Clear all interrupt enables
     */
    PIE0 = 0x00;
    PIE2 = 0x00;
    PIE3 = 0x00;
    PIE4 = 0x00;
    PIE5 = 0x00;
    PIE6 = 0x00;
    PIE7 = 0x00;
    PIE8 = 0x00;
    PIE9 = 0x00;
    PIE10 = 0x00;
    PIE11 = 0x00;
    PIE12 = 0x00;
    PIE13 = 0x00;
    PIE14 = 0x00;
    PIE15 = 0x00;

    /*
     * Clear all interrupt requests
     */
    PIR0 = 0x00;
    PIR1 = 0x00;
    PIR2 = 0x00;
    PIR3 = 0x00;
    PIR4 = 0x00;
    PIR5 = 0x00;
    PIR6 = 0x00;
    PIR7 = 0x00;
    PIR8 = 0x00;
    PIR9 = 0x00;
    PIR10 = 0x00;
    PIR11 = 0x00;
    PIR12 = 0x00;
    PIR13 = 0x00;
    PIR14 = 0x00;
    PIR15 = 0x00;

    /*
     * Disable all peripheral modules until after PPS mapping and I/O setup is done,
     * then selectively enable the required modules to save power.
     */
    PMD0 = 0xFF;
    PMD1 = 0xFF;
    PMD3 = 0xFF;
    PMD4 = 0xFF;
    PMD5 = 0xFF;
    PMD6 = 0xFF;
    PMD7 = 0xFF;
    PMD8 = 0xFF;

    // Set up port C
    //
    // This port is the LED outputs based on the switch inputs on
    // port D.
    TRISC = 0x00;   // All pins output
    ANSELC = 0x00;  // All digital
    LATC = 0xFF;    // All LEDs off (active low)
    ODCONC = 0x00;  // No open-drain (LEDs will be driven high or low by the MCP23017)
    WPUD = 0x00;    // No pull-ups needed on port D since they are outputs to the LEDs
    SLRCOND = 0x00; // No slew rate control needed for LED outputs
    INLVLD = 0x00;  // No input level control needed for outputs

    // Set up port D
    //
    // This port is all digital inputs with weak pull-ups enabled and interrupt on change configured for the encoder inputs and the "enter" button. The I2C lines are bit-banged on port C, so they are not configured as I2C module pins.
    TRISD = 0xFF;  // All pins input
    ANSELD = 0x00;
    LATD = 0x00;
    ODCOND = 0x00; // No open-drain 
    WPUD = 0xFF;
    INLVLD = 0x00;
    SLRCOND = 0x00;

    /*
     * Enable all peripheral modules that are required
     */
    PMD0bits.SYSCMD = 0; // System clock network enabled
    PMD0bits.IOCMD = 0;  // Interrupt on change module enabled

    // IOC on all PORTD pins for both edges.
    IOCDP = 0xFF;
    IOCDN = 0xFF;
    IOCDF = 0x00;
    PIR0bits.IOCIF = 0;
    PIE0bits.IOCIE = 1;

    // Initialize output state so PORTC mirrors inverted PORTD at startup.
    LATC = (uint8_t)(~PORTD);

    /*
     * Enable global interrupts
     */
    INTCON0bits.GIE = 1; // Interrupts enabled

    system_initialized = true;
}