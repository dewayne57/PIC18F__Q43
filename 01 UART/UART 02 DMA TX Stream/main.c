/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for UART 02 DMA TX Stream.
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
 *   This file contains the main function for the UART 02 DMA TX Stream project. It
 *   initializes the system configuration and starts the main application loop. The 
 *   main loop periodically sends a test message over UART using the DMA transmission 
 *   implemented in the uart_dma_tx module.
 * ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include "config.h"
#include "uart_dma_tx.h"

void main(void)
{
    UART_Initialize();

    while (true)
    {
        __delay_ms(1000);
        printf("Hello, UART DMA TX Stream!\r\n");
    }
}
