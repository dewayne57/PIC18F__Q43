/* ****************************************************************************************
 *   File Name: ioc.c
 *   Description: This file contains the Interrupt-On-Change (IOC) handling routines for
 *   the IOC Single project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-09
 * 
 *   Copyright (c) 2026, Dewayne Hafenstein.
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 ***************************************************************************************** */

#include <xc.h>
#include "uart.h"

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
    if ((PIE4bits.U1RXIE != 0U) && (PIR4bits.U1RXIF != 0U))
    {
        UART1_RX_ISR();
    }

    if ((PIE4bits.U1TXIE != 0U) && (PIR4bits.U1TXIF != 0U))
    {
        UART1_TX_ISR();
    }

    if (PIR0bits.IOCIF != 0)
    {
        LATD = (uint8_t)(~PORTC);
        IOCCF = 0x00;
        PIR0bits.IOCIF = 0;
    }
}
