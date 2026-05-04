/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for UART 02 DMA TX Stream.
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
***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "uart_dma_tx.h"}

/// @brief  Initializes the system, including oscillator, pin configuration, UART, and DMA 
/// for UART transmission. This function should be called once at the start of the main 
/// function to set up the microcontroller for operation. It configures the necessary hardware 
/// modules and enables global interrupts to allow the UART DMA transmission to function 
/// correctly.
/// @param None
/// @return None    
void UART_Initialize(void)
{
    INTCON0bits.GIE = 0;

    /* Keep all IO pins digital for bring-up. */
    ANSELB = 0x00;
    ANSELC = 0x00;
    ANSELD = 0x00;

    /* UART1 TX pin (RC6) as digital output. */
    TRISCbits.TRISC6 = 0;

    /* Map UART1 TX function to RC6 pin */
    RC6PPS = 0x20U;

    /* Initialize UART1 */
    U1CON0 = 0x20U; /* TX enabled, async 8-bit mode defaults. */
    U1CON1 = 0x80U; /* UART enabled. */
    U1BRG = (uint16_t)((_XTAL_FREQ / (4UL * _UART_BAUD)) - 1UL);

    /* Initialize DMA1 for UART TX. */
    PMD4bits.DMA1MD = 0;
    DMA1CON0 = 0x00U;
    DMA1CON1 = 0x00U;

    INTCON0bits.IPEN = 1U;
    DMA1SCNTIP = 0U;
    DMA1DCNTIP = 0U;
    DMA1ORIP = 0U;
    DMA1AIP = 0U;

    DMA1SCNTIF = 0U;
    DMA1DCNTIF = 0U;
    DMA1AIF = 0U;
    DMA1ORIF = 0U;
    DMA1SCNTIE = 1U;
    DMA1DCNTIE = 1U;
    DMA1AIE = 1U;
    DMA1ORIE = 1U;

    /* and finally initialize UART DMA TX state */
    UART_DMA_TX_StateInitialize();

    /* Global interrupt enable. */
    INTCON0bits.GIEH = 1;
    INTCON0bits.GIEL = 1;
}
