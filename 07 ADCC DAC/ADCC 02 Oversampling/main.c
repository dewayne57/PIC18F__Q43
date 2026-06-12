/* *****************************************************************************************
 *   File Name: main.c
 *   Description: ADCC oversampling with hardware averaging and UART reporting.
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

static volatile uint16_t adcAveragedCounts;
static volatile bool reportAveragedSample;

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

#if defined(VECTORED_INTERRUPTS_ENABLED)
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
{
    UART_HandleRxInterrupt(&console_uart);
}

void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
{
    UART_HandleTxInterrupt(&console_uart);
}

// @brief ADC interrupt service routine.  Fires once per ADCC_OVERSAMPLE_COUNT
/// conversions when the hardware Average mode result is ready in ADFLTR.
/// The ISR copies the hardware-averaged 12-bit value for UART reporting.
void __interrupt(irq(IRQ_AD), high_priority) ADC_ISR(void)
{
    if (PIR1bits.ADIF)
    {
        adcAveragedCounts = ADFLTR; // Hardware has already averaged ADCC_OVERSAMPLE_COUNT samples
        PIR1bits.ADIF = 0;
        reportAveragedSample = true;
    }
}
#else
/// @brief Legacy (flat) interrupt service routine.  Handles all interrupts, including UART and ADC.
/// The ADC interrupt fires once per ADCC_OVERSAMPLE_COUNT conversions when the hardware Average mode
/// result is ready in ADFLTR. The ISR copies the hardware-averaged 12-bit value for UART reporting.
/// @param  None
/// @return  None
void __interrupt() NonVectoredISR(void)
{
    if (PIR4bits.U1RXIF)
    {
        UART_HandleRxInterrupt(&console_uart);
    }

    if (PIR4bits.U1TXIF)
    {
        UART_HandleTxInterrupt(&console_uart);
    }

    if (PIR1bits.ADIF)
    {
        adcAveragedCounts = ADFLTR; // Hardware has already averaged ADCC_OVERSAMPLE_COUNT samples
        PIR1bits.ADIF = 0;
        reportAveragedSample = true;
    }
}
#endif

/// @brief Main application entry point.
/// @param  None
/// @return  None
void main(void)
{
    uint16_t triggerCountdownMs;

    adcAveragedCounts = 0U;
    reportAveragedSample = false;
    triggerCountdownMs = 0U; // Trigger immediately at startup, then every ADCC_TRIGGER_PERIOD_MS

    SYSTEM_Initialize();
    if (!UART_Open(&console_uart))
    {
        while (1)
        {
        }
    }
    UART_SelectPrintfTarget(&console_uart);

    printf("ADCC 02 Oversampling Filtering%s", CRLF);
    printf("HW avg samples=%u (CRS=%u), SW trigger=%u ms%s",
           ADCC_OVERSAMPLE_COUNT,
           ADCC_OVERSAMPLE_CRS,
           ADCC_TRIGGER_PERIOD_MS,
           CRLF);

    while (1)
    {
        if (reportAveragedSample)
        {
            uint16_t sampleCounts;
            uint32_t sampleMv;

            INTCON0bits.GIEH = 0;
            sampleCounts = adcAveragedCounts;
            reportAveragedSample = false;
            INTCON0bits.GIEH = 1;

            sampleMv = ADCC_COUNTS_TO_MV(sampleCounts);
            printf("Averaged Input: %4u counts (%4lu mV)%s", sampleCounts, sampleMv, CRLF);
        }

        // Start a new hardware average conversion group whenever the trigger countdown reaches zero and the
        // ADC is not already busy with a conversion.  We will not wait for it to complete here - the ADC interrupt
        // will fire when the result is ready, and the ISR will copy the result for reporting in the main loop.
        // The main loop just manages the trigger cadence and reporting of completed results, while the ADC hardware
        // and ISR handle the oversampling and averaging autonomously in the background.
        if ((triggerCountdownMs == 0U) && (ADCON0bits.GO == 0))
        {
            PIR1bits.ADIF = 0; // Ensure we only react to the next completed trigger
            ADCON0bits.GO = 1; // Start one ADCC hardware-average conversion group
        }

        // Simple software timer for managing the trigger cadence.  The main loop is not doing anything else,
        // so a blocking delay is sufficient.
        while (triggerCountdownMs > 0U)
        {
            __delay_ms(1);
            triggerCountdownMs--;
        }
        triggerCountdownMs = ADCC_TRIGGER_PERIOD_MS; // Reset the trigger countdown for the next cycle
    }
}
