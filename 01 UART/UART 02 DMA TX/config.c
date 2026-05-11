/* *****************************************************************************************
 *   File Name: config.c
 *   Description: System initialization for UART 02 DMA TX.
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
#include "dma.h"
#include "uart_dma_tx.h"

/* DMA hardware trigger ID for UART1 TX.  When the UART1 TX hardware FIFO has space,
   it automatically asserts trigger 0x21, which tells DMA1 to copy the next source byte.
   This value comes from the PIC18F47Q43 datasheet DMA trigger source table. */
#define DMA_UART1_TX_TRIGGER 0x21U

/// @brief Apply configured UART frame format to MODE and stop-bit settings.
/// @param None
/// @return None
static void UART1_ApplyFrameFormat(void)
{
    U1CON0bits.MODE = 0x0;

    if (UART_1_DATA_BITS == 9)
    {
        U1CON0bits.MODE = 0x2;
    }
    else if (UART_1_PARITY == UART_PARITY_ODD)
    {
        U1CON0bits.MODE = 0x8;
    }
    else if (UART_1_PARITY == UART_PARITY_EVEN)
    {
        U1CON0bits.MODE = 0x9;
    }

    switch (UART_1_STOP_BITS)
    {
        case UART_STOP_BITS_1_5:
        case UART_STOP_BITS_2:
            U1CON2bits.STP = UART_1_STOP_BITS;
            break;

        case UART_STOP_BITS_1:
        default:
            U1CON2bits.STP = UART_STOP_BITS_1;
            break;
    }
}

/// @brief Initialize clocking, pins, UART1, DMA1, and interrupts.
/// @param None
/// @return None
void SYSTEM_Initialize(void)
{
    INTCON0bits.GIEH = 0;  // Disable high priority interrupts during initialization.
    INTCON0bits.GIEL = 0;  // Disable low priority interrupts during initialization.
    /* IPEN=1 enables interrupt priority levels so that DMA completion ISRs can be assigned
       low priority, allowing higher-priority application interrupts to preempt them. */
    INTCON0bits.IPEN = 1U; // Enable interrupt priority levels.

    /* Keep all demonstration pins digital. */
    ANSELB = 0x00;
    ANSELC = 0x00;
    ANSELD = 0x00;

    /* UART1 TX pin on RB0, UART1 RX pin on RB1. */
    TRISBbits.TRISB0 = 0U;
    TRISBbits.TRISB1 = 1U;

    // Unlock the PPS registers to configure pin mapping.
    PPSLOCK = 0x55U;            // Unlock sequence part 1
    PPSLOCK = 0xAAU;            // Unlock sequence part 2
    PPSLOCKbits.PPSLOCKED = 0U; // Ensure PPS is unlocked

    /* Map UART1 TX to RB0 and UART1 RX from RB1. */
    RB0PPS = 0x20U;
    U1RXPPS = 0x09U;

    // Lock the PPS registers after configuration.
    PPSLOCK = 0x55U;            // Lock sequence part 1
    PPSLOCK = 0xAAU;            // Lock sequence part 2
    PPSLOCKbits.PPSLOCKED = 1U; // Lock PPS to prevent further changes
    
    /* PMD (Peripheral Module Disable) registers allow the CPU to power down unused
       peripherals to save current.  Setting a PMD bit to 0 keeps the module ENABLED.
       Explicitly clear these bits to ensure UART1 and DMA1 are active even if they
       were gated off by a bootloader or reset condition. */
    PMD6bits.U1MD = 0U;   // UART1 module enable (0 = enabled)
    PMD8bits.DMA1MD = 0U; // DMA channel 1 module enable (0 = enabled)

    /* Initialize UART1 using the configured framing. */
    U1CON0 = 0x00U;
    U1CON2 = 0x00U;
    UART1_ApplyFrameFormat();
    U1CON0bits.TXEN = 1U;
    U1CON1 = 0x80U; /* UART1 module ON (U1CON1<7> = UART ON bit). */
    /* High-speed baud generator mode (BRGS=1): baud rate = Fosc / (4 * (U1BRG + 1))
       This halves the divisor compared to standard mode, enabling higher baud rates
       or finer resolution at lower baud rates.  The BRG value in config.h was calculated
       for this mode so both must be kept in sync. */
    U1BRGS = 1U;
    U1BRG = (uint16_t)UART_1_BRG_VALUE; // Load pre-calculated baud rate divisor

    /* Initialize DMA1 for UART TX. */
    PMD8bits.DMA1MD = 0U;
    DMA_SelectChannel(1U);
    DMAnCON0 = 0x00U;
    DMAnCON0bits.EN = 1U;     // DMA channel enabled but not yet triggered (DGO=0)
    DMAnCON0bits.SIRQEN = 1U; // Start IRQ enabled: hardware trigger (UART1 TX) advances the DMA
    DMAnCON0bits.AIRQEN = 0U; // Abort IRQ disabled: no hardware event will abort the transfer
    DMAnCON1 = 0x00U;
    /* SMODE=1: source address auto-increments after each byte copied (walks through TX buffer).
       DMODE=0: destination address stays fixed (always writes to U1TXB hardware register).
       SSTP=1:  DMA halts automatically when the source byte count reaches zero - one-shot mode. */
    DMAnCON1bits.SMODE = 1U;  // Source pointer increments after each byte transferred
    DMAnCON1bits.DMODE = 0U;  // Destination pointer fixed (always targets U1TXB)
    DMAnCON1bits.SSTP = 1U;   // Auto-stop when source byte count is exhausted
    DMAnAIRQ = 0x00U;         // No abort-trigger source
    DMAnSIRQ = DMA_UART1_TX_TRIGGER; // Hardware trigger: UART1 TX FIFO has space
    DMAnSCNT = 0U;            // Source count register cleared (set before each transfer)
    DMAnDCNT = 0U;            // Destination count not used in fixed-destination mode

    /* All DMA1 interrupts are set to low priority (0) so that any higher-priority ISR
       can preempt DMA housekeeping.  The DMA ISRs only update software state and restart
       the next buffer transfer, so low priority is appropriate. */
    IPR2bits.DMA1SCNTIP = 0U; // Source-count done interrupt: low priority
    IPR2bits.DMA1DCNTIP = 0U; // Destination-count done interrupt: low priority
    IPR2bits.DMA1ORIP = 0U;   // Overrun error interrupt: low priority
    IPR2bits.DMA1AIP = 0U;    // Abort interrupt: low priority

    /* Clear any stale interrupt flags before enabling, then enable all four DMA1 interrupts.
       SCNT fires when the source buffer is exhausted (transfer complete - normal case).
       DCNT fires when destination count is exhausted (not commonly used here).
       AIF  fires if the DMA is aborted by an error or software.
       ORIF fires if DMA tries to write when the destination is not ready (UART overrun). */
    PIR2bits.DMA1SCNTIF = 0U; // Clear source-count flag
    PIR2bits.DMA1DCNTIF = 0U; // Clear destination-count flag
    PIR2bits.DMA1AIF = 0U;    // Clear abort flag
    PIR2bits.DMA1ORIF = 0U;   // Clear overrun flag
    PIE2bits.DMA1SCNTIE = 1U; // Enable source-count interrupt (transfer complete)
    PIE2bits.DMA1DCNTIE = 1U; // Enable destination-count interrupt
    PIE2bits.DMA1AIE = 1U;    // Enable abort interrupt
    PIE2bits.DMA1ORIE = 1U;   // Enable overrun interrupt

    /* Ensure DGO is cleared before state init. */
    DMA_SelectChannel(1U);
    DMAnCON0bits.DGO = 0U;

    /* Initialize UART DMA TX state. */
    UART_DMA_TX_StateInitialize();

    /* Global interrupt enable. */
    INTCON0bits.GIEH = 1;
    INTCON0bits.GIEL = 1;
}
