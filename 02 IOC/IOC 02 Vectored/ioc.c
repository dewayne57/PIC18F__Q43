/* ****************************************************************************************
 *   File Name: ioc.c
 *   Description: This file contains the Interrupt-On-Change (IOC) handling routines for
 *   the IOC Single project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-09
 ***************************************************************************************** */

#include <xc.h>

/**
 * Interrupt Service Routine (ISR) for handling Interrupt-On-Change (IOC) events. This function is
 * called when an IOC event occurs on any of the configured pins. The specific handling code for
 * each pin should be implemented within this function based on the application's requirements.
 *
 * This ISR (Interrupt Service Routine) should be registered in the interrupt vector table to ensure it is
 * called when an IOC event occurs. The function should check which pin triggered the interrupt and
 * handle the event accordingly, then clear the interrupt flag(s) to allow for future interrupts.
 *
 * Note this is a vectored interrupt, which means this routine will only be called for the specific
 * interrupt source(s) it is registered for.  In this project, it is registered for the IOC events,
 * so it will only be called when an IOC event occurs.  Note however if you have multiple ports 
 * configured for IOC, you will need to check the specific IOC flags to determine which port/pin 
 * caused the interrupt.
 * The actual handling of the IOC event (e.g., reading the pin state, updating variables, etc.)
 * should be implemented within this function based on the application's requirements. After
 * handling the event, the interrupt flag(s) should be cleared to allow for future interrupts.
 *
 */
void __interrupt(irq(0x07), low_priority) ISR(void)
{
    LATD = (uint8_t)(~PORTC);
    IOCCF = 0x00;
    PIR0bits.IOCIF = 0;
}
