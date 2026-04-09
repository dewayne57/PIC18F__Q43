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
 */
void __interrupt() ISR(void)
{
    if (PIR0bits.IOCIF != 0)
    {
        LATC = (uint8_t)(~PORTD);
        IOCDF = 0x00;
        PIR0bits.IOCIF = 0;
    }
}
