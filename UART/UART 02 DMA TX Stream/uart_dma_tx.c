/* *****************************************************************************************
 *   File Name: uart_dma_tx.c
 *   Description: UART1 transmit using DMA where available, with fallback behavior.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "config.h"
#include "uart_dma_tx.h"

static const uint8_t s_tx_frame[] = "UART DMA TX stream\r\n";

static volatile uint8_t s_dma_busy;
static volatile uint16_t s_dma_events;
static volatile uint16_t s_gap_ticks;

static void UART1_Initialize(void)
{
#ifdef RC6PPS
    RC6PPS = 0x20U;
#endif

#ifdef U1CON0
    U1CON0 = 0x20U; /* TX enabled, async 8-bit mode defaults. */
#endif
#ifdef U1CON1
    U1CON1 = 0x80U; /* UART enabled. */
#endif
#ifdef U1BRG
    U1BRG = (uint16_t)((APP_FOSC_HZ / (4UL * APP_UART_BAUD)) - 1UL);
#endif
}

static void DMA1_Initialize(void)
{
#ifdef PMD4bits
    PMD4bits.DMA1MD = 0;
#endif

#ifdef DMA1CON0
    DMA1CON0 = 0x00U;
#endif
#ifdef DMA1CON1
    DMA1CON1 = 0x00U;
#endif
}

static void DMA1_SetSourceAddress(const uint8_t *data)
{
    uintptr_t addr = (uintptr_t)data;

#ifdef DMA1SSAL
    DMA1SSAL = (uint8_t)(addr & 0xFFU);
#endif
#ifdef DMA1SSAH
    DMA1SSAH = (uint8_t)((addr >> 8) & 0xFFU);
#endif
#ifdef DMA1SSAU
    DMA1SSAU = (uint8_t)((addr >> 16) & 0xFFU);
#endif
}

static void DMA1_SetDestAddress(void)
{
#if defined(U1TXB)
    uintptr_t addr = (uintptr_t)(&U1TXB);
#else
    uintptr_t addr = 0U;
#endif

#ifdef DMA1DSAL
    DMA1DSAL = (uint8_t)(addr & 0xFFU);
#endif
#ifdef DMA1DSAH
    DMA1DSAH = (uint8_t)((addr >> 8) & 0xFFU);
#endif
#ifdef DMA1DSAU
    DMA1DSAU = (uint8_t)((addr >> 16) & 0xFFU);
#endif
}

static void DMA1_SetTransferCount(uint16_t count)
{
#ifdef DMA1SSZL
    DMA1SSZL = (uint8_t)(count & 0xFFU);
#endif
#ifdef DMA1SSZH
    DMA1SSZH = (uint8_t)((count >> 8) & 0xFFU);
#endif
#ifdef DMA1DSZL
    DMA1DSZL = 1U;
#endif
#ifdef DMA1DSZH
    DMA1DSZH = 0U;
#endif
}

static void DMA1_KickUartTx(const uint8_t *data, uint16_t count)
{
    DMA1_SetSourceAddress(data);
    DMA1_SetDestAddress();
    DMA1_SetTransferCount(count);

#ifdef DMA1CON0bits
    DMA1CON0bits.EN = 1;
    DMA1CON0bits.DGO = 1;
    s_dma_busy = 1U;
#else
    /* Fallback path for environments without DMA symbols. */
    (void)data;
    (void)count;
    s_dma_busy = 0U;
    s_dma_events++;
#endif
}

void UART_DMA_TX_Initialize(void)
{
    s_dma_busy = 0U;
    s_dma_events = 0U;
    s_gap_ticks = 0U;

    UART1_Initialize();
    DMA1_Initialize();
}

void UART_DMA_TX_Task(void)
{
    if (s_dma_busy == 0U)
    {
        if (s_gap_ticks >= 15000U)
        {
            DMA1_KickUartTx(s_tx_frame, (uint16_t)(sizeof(s_tx_frame) - 1U));
            s_gap_ticks = 0U;
        }
        else
        {
            s_gap_ticks++;
        }
    }

#ifdef DMA1CON0bits
    if ((s_dma_busy != 0U) && (DMA1CON0bits.DGO == 0U))
    {
        s_dma_busy = 0U;
        s_dma_events++;
    }
#endif

    if ((s_dma_events & 0x001FU) == 0U)
    {
        LATDbits.LATD0 = 1U;
    }
    else
    {
        LATDbits.LATD0 = 0U;
    }
}
