/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for UART 03 DMA RX.
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
 * ***************************************************************************************** */

#include <xc.h>
#include "config.h"
#include "uart_dma_rx.h"
#include "uart_dma_tx.h"

void UART_Initialize(void)
{
    INTCON0bits.GIE = 0U;

    /* Keep all IO pins digital for bring-up. */
    ANSELB = 0x00U;
    ANSELC = 0x00U;
    ANSELD = 0x00U;

    /* UART1 pins: RX on RB0, TX on RB1, RTS on RB2, CTS on RB3. */
    TRISBbits.TRISB0 = 1U;
    TRISBbits.TRISB1 = 0U;
    TRISBbits.TRISB2 = 0U;
    TRISBbits.TRISB3 = 1U;

    RB1PPS = 0x20U;
    RB2PPS = 0x22U;
    U1RXPPS = 0x08U;
    U1CTSPPS = 0x0BU;

    /* UART enabled in async mode with RTS/CTS hardware flow control. */
    U1CON0 = 0xB0U;
    U1CON1 = 0x80U;
    U1CON2 = 0x83U;
    U1BRG = (uint16_t)((_XTAL_FREQ / (4UL * _UART_BAUD)) - 1UL);

    /* Initialize DMA1 used for UART RX buffer capture. */
    PMD4bits.DMA1MD = 0U;
    DMA1CON0 = 0x00U;
    DMA1CON1 = 0x00U;

    /* Initialize DMA2 used for UART TX streaming. */
    PMD4bits.DMA2MD = 0U;
    DMA2CON0 = 0x00U;
    DMA2CON1 = 0x00U;

    INTCON0bits.IPEN = 1U;
    DMA1SCNTIP = 0U;
    DMA1DCNTIP = 0U;
    DMA1ORIP = 0U;
    DMA1AIP = 0U;
    DMA2SCNTIP = 0U;
    DMA2DCNTIP = 0U;
    DMA2ORIP = 0U;
    DMA2AIP = 0U;

    DMA1SCNTIF = 0U;
    DMA1DCNTIF = 0U;
    DMA1AIF = 0U;
    DMA1ORIF = 0U;
    DMA2SCNTIF = 0U;
    DMA2DCNTIF = 0U;
    DMA2AIF = 0U;
    DMA2ORIF = 0U;
    DMA1SCNTIE = 1U;
    DMA1DCNTIE = 1U;
    DMA1AIE = 1U;
    DMA1ORIE = 1U;
    DMA2SCNTIE = 1U;
    DMA2DCNTIE = 1U;
    DMA2AIE = 1U;
    DMA2ORIE = 1U;

    // Initialize application state for UART DMA RX and TX modules.
    UART_DMA_RX_StateInitialize();
    UART_DMA_TX_StateInitialize();

    INTCON0bits.GIEH = 1U;
    INTCON0bits.GIEL = 1U;
}
