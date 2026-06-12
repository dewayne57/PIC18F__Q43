/* *****************************************************************************************
 *   File Name: main.c
 *   Description: DAC reference ladder sweep with ADCC readback and UART reporting.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-06-12
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
static volatile uint16_t dacSetpointMv;
static volatile uint8_t dacSetpointCounts;
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

static void DAC1_WriteMillivolts(uint16_t targetMv)
{
    uint8_t dacCounts = (uint8_t)DAC_MV_TO_COUNTS(targetMv);

    DAC1DATL = dacCounts;
    dacSetpointMv = targetMv;
    dacSetpointCounts = dacCounts;
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

void __interrupt(irq(IRQ_AD), high_priority) ADC_ISR(void)
{
    if (PIR1bits.ADIF)
    {
        adcAveragedCounts = ADFLTR;
        PIR1bits.ADIF = 0;
        reportAveragedSample = true;
    }
}
#else
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
        adcAveragedCounts = ADFLTR;
        PIR1bits.ADIF = 0;
        reportAveragedSample = true;
    }
}
#endif

void main(void)
{
    uint16_t triggerCountdownMs = 0U;
    uint16_t nextDacMv = DAC_LADDER_MIN_MV;
    bool rampUp = true;

    adcAveragedCounts = 0U;
    dacSetpointMv = DAC_LADDER_MIN_MV;
    dacSetpointCounts = (uint8_t)DAC_MV_TO_COUNTS(DAC_LADDER_MIN_MV);
    reportAveragedSample = false;

    SYSTEM_Initialize();
    if (!UART_Open(&console_uart))
    {
        while (1)
        {
        }
    }
    UART_SelectPrintfTarget(&console_uart);

    printf("DAC 01 Reference Ladder%s", CRLF);
    printf("DAC step=%u mV every %u ms, ADCC avg=%u samples%s",
           DAC_LADDER_STEP_MV,
           DAC_STEP_PERIOD_MS,
           ADCC_OVERSAMPLE_COUNT,
           CRLF);

    while (1)
    {
        if (reportAveragedSample)
        {
            uint16_t sampleCounts;
            uint16_t setpointMv;
            uint8_t setpointCounts;
            uint32_t sampleMv;

            INTCON0bits.GIEH = 0;
            sampleCounts = adcAveragedCounts;
            setpointMv = dacSetpointMv;
            setpointCounts = dacSetpointCounts;
            reportAveragedSample = false;
            INTCON0bits.GIEH = 1;

            sampleMv = ADCC_COUNTS_TO_MV(sampleCounts);
            printf("DAC: %4u mV (%3u)  ADCC: %4u counts (%4lu mV)%s",
                   setpointMv,
                   setpointCounts,
                   sampleCounts,
                   sampleMv,
                   CRLF);
        }

        if ((triggerCountdownMs == 0U) && (ADCON0bits.GO == 0))
        {
            DAC1_WriteMillivolts(nextDacMv);

            PIR1bits.ADIF = 0;
            ADCON0bits.GO = 1;
            triggerCountdownMs = DAC_STEP_PERIOD_MS;

            if (rampUp)
            {
                if ((nextDacMv + DAC_LADDER_STEP_MV) >= DAC_LADDER_MAX_MV)
                {
                    nextDacMv = DAC_LADDER_MAX_MV;
                    rampUp = false;
                }
                else
                {
                    nextDacMv += DAC_LADDER_STEP_MV;
                }
            }
            else
            {
                if (nextDacMv <= (DAC_LADDER_MIN_MV + DAC_LADDER_STEP_MV))
                {
                    nextDacMv = DAC_LADDER_MIN_MV;
                    rampUp = true;
                }
                else
                {
                    nextDacMv -= DAC_LADDER_STEP_MV;
                }
            }
        }

        __delay_ms(1);
        if (triggerCountdownMs > 0U)
        {
            triggerCountdownMs--;
        }
    }
}
