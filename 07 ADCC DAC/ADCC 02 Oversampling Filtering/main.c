/* *****************************************************************************************
 *   File Name: main.c
 *   Description: ADCC oversampling with averaging filter and UART reporting.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-06-10
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
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "../../Libraries/UARTLIB/uartlib.h"

static volatile uint16_t adcFilteredCounts;
static volatile bool adcFilterInitialized;
static volatile bool reportFilteredSample;

static char console_tx_buffer[256];
static char console_rx_buffer[128];

static uart_handle_t console_uart = {
    .port = UART_PORT_1,
    .high_speed_baud = false,
    .baud_rate = 19200U,
    .fosc = _XTAL_FREQ,
    .data_bits = 8U,
    .parity = UART_PARITY_NONE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_control = UART_FLOW_NONE,
    .tx_buffer = console_tx_buffer,
    .tx_buffer_size = sizeof(console_tx_buffer),
    .rx_buffer = console_rx_buffer,
    .rx_buffer_size = sizeof(console_rx_buffer),
    .tx_head = 0U,
    .tx_tail = 0U,
    .rx_head = 0U,
    .rx_tail = 0U,
    .initialized = false,
    .tx_pin = UART_PPS_PIN_RB0,
    .rx_pin = UART_PPS_PIN_RB1,
    .rts_pin = UART_PPS_PIN_NONE,
    .cts_pin = UART_PPS_PIN_NONE,
    .isr_mode = UART_ISR_VECTORED,
};

static uint16_t ApplyIirFilter(uint16_t previousCounts, uint16_t newCounts)
{
    uint32_t weighted = ((uint32_t)previousCounts * (ADCC_FILTER_ALPHA_DEN - ADCC_FILTER_ALPHA_NUM)) + ((uint32_t)newCounts * ADCC_FILTER_ALPHA_NUM) + (ADCC_FILTER_ALPHA_DEN / 2U);
    return (uint16_t)(weighted / ADCC_FILTER_ALPHA_DEN);
}

#if defined(VECTORED_INTERRUPTS_ENABLED)
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
{
    UART_HandleRxInterrupt(&console_uart);
}

void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
{
    UART_HandleTxInterrupt(&console_uart);
}
#endif

/// @brief ADC interrupt service routine.  Fires once per ADCC_OVERSAMPLE_COUNT
/// conversions when the hardware Average mode result is ready in ADFLTR.
/// The hardware-averaged 12-bit value is passed through a software IIR filter
/// for additional noise rejection before signalling the main loop.
void __interrupt(irq(IRQ_AD), high_priority) ADC_ISR(void)
{
    if (PIR1bits.ADIF)
    {
        uint16_t hwAveraged = ADFLTR; // Hardware has already averaged ADCC_OVERSAMPLE_COUNT samples
        PIR1bits.ADIF = 0;

        if (!adcFilterInitialized)
        {
            adcFilteredCounts = hwAveraged; // Seed the IIR filter with the first hardware average
            adcFilterInitialized = true;
        }
        else
        {
            adcFilteredCounts = ApplyIirFilter(adcFilteredCounts, hwAveraged);
        }

        reportFilteredSample = true;
    }
}

void main(void)
{
    adcFilteredCounts = 0U;
    adcFilterInitialized = false;
    reportFilteredSample = false;

    SYSTEM_Initialize();
    if (!UART_Open(&console_uart))
    {
        while (1)
        {
        }
    }
    UART_SelectPrintfTarget(&console_uart);

    printf("ADCC 02 Oversampling Filtering%s", CRLF);
    printf("HW avg samples=%u (CRS=%u), IIR alpha=%u/%u%s",
           ADCC_OVERSAMPLE_COUNT,
           ADCC_OVERSAMPLE_CRS,
           ADCC_FILTER_ALPHA_NUM,
           ADCC_FILTER_ALPHA_DEN,
           CRLF);

    while (1)
    {
        if (reportFilteredSample)
        {
            uint16_t sampleCounts;
            uint32_t sampleMv;

            INTCON0bits.GIEH = 0;
            sampleCounts = adcFilteredCounts;
            reportFilteredSample = false;
            INTCON0bits.GIEH = 1;

            sampleMv = ADCC_COUNTS_TO_MV(sampleCounts);
            printf("Filtered Input: %4u counts (%4lu mV)%s", sampleCounts, sampleMv, CRLF);

            __delay_ms(ADCC_REPORT_PERIOD_MS);
        }
    }
}
