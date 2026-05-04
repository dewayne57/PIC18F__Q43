/* *****************************************************************************************
 *   File Name: config.h
 *   Description: Device config and system init declarations for UART 03 DMA RX.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-01
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
 *   declarations for the UART 03 DMA RX project. It sets up the microcontroller's
 *   oscillator and pin configurations, then initializes UART and DMA for receive.
 * ***************************************************************************************** */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* PIC18F Q43 Configuration Bit Settings */
#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_64MHZ
#pragma config CSWEN = OFF
#pragma config FCMEN = OFF
#pragma config MVECEN = 1
#pragma config WDTE = OFF
#pragma config MCLRE = EXTMCLR
#pragma config LVP = ON
#pragma config XINST = OFF
#pragma config PPS1WAY = OFF

#define _XTAL_FREQ (64000000L)
#define _UART_BAUD (115200L)
#define UART_BUFFER_SIZE (256U)

void UART_Initialize(void);

#endif /* CONFIG_H */
