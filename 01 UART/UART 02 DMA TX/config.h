/* *****************************************************************************************
 *   File Name: config.h
 *   Description: Device config and system init declarations for UART 02 DMA TX Stream.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 *
 *   Copyright (c) 2026, Dewayne Hafenstein.
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 *   This file provides configuration bit settings and system initialization function
 *   declarations for the UART 02 DMA TX Stream project. It sets up the microcontroller's
 *   oscillator, pin configurations, and initializes the UART and DMA modules for use in
 *   the application.  All DMA processing is handled in the uart_dma_tx module, and this
 *   config file serves as the central point for system configuration and initialization.
 * ***************************************************************************************** */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* PIC18F27Q43 Configuration Bit Settings */
#pragma config FEXTOSC = OFF           /* External Oscillator mode selection bits (Oscillator not enabled) */
#pragma config RSTOSC = HFINTOSC_64MHZ /* Power-up default value for COSC bits (HFINTOSC with 64 MHz) */
#pragma config CSWEN = OFF             /* Clock Switch Enable bit (Clock switching is disabled) */
#pragma config FCMEN = OFF             /* Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor is \
                                        disabled) */
#pragma config MVECEN = 1              /* Multi-vector Enable bit (Multi-vector enabled) */
#pragma config WDTE = OFF              /* Watchdog Timer Enable bits (WDT disabled) */
#pragma config MCLRE = EXTMCLR         /* MCLR Pin Enable bit (MCLR pin enabled, RE3 input pin disabled) */
#pragma config LVP = ON                /* Low-Voltage Programming Enable bit (Low-voltage programming \
                                        enabled) */
#pragma config XINST = OFF             /* Extended Instruction Set Enable bit (Extended Instruction Set \
                                           and Indexed Addressing Mode disabled) */
#pragma config PPS1WAY = OFF           /* Peripheral Pin Select one-way control bit (The PPSLOCK bit can be \
                                           set and cleared repeatedly by software) */

#define _XTAL_FREQ (64000000L)
#define _UART_BAUD (115200L)
#define UART_BUFFER_SIZE (256 - 1)

void UART_Initialize(void);

#endif /* CONFIG_H */
