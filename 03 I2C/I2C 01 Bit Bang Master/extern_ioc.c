/* *****************************************************************************************
 *   File Name: extern_ioc.c
 *   Description: External interrupt on change (IOC) handler for RB2.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-11
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
 *   
 *   Pin Configuration:
 *   RB2 - External interrupt input with IOC enabled on both rising and falling edges
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "extern_ioc.h"

extern uart_handle_t console_uart;

void ExternIoc_Initialize(void)
{
    // Configure RB2 as digital input with weak pull-up
    TRISBbits.TRISB2 = 1;       // RB2 is input
    ANSELBbits.ANSELB2 = 0;     // RB2 is digital
    WPUBbits.WPUB2 = 1;         // Weak pull-up enabled on RB2

    // Enable IOC on RB2 for both rising and falling edges
    IOCBbits.IOCB2 = 1;         // IOC enabled on RB2

    // Clear any pending IOC interrupt flags
    IOCCF = 0x00;
    PIR0bits.IOCIF = 0;
}

void ExternIoc_HandleInterrupt(void)
{
    // Read current state of RB2
    uint8_t rb2_state = PORTBbits.RB2;

    // Application-specific handling of RB2 state change
    // This is a simple diagnostic example that outputs the state to UART
    printf("RB2 External IOC: State = %d\r\n", rb2_state);

    // Clear the IOC interrupt flag
    IOCCF = 0x00;
    PIR0bits.IOCIF = 0;
}
