/* *****************************************************************************************
 *   File Name: main.c
 *   Description: DAC software servo using ADCC setpoint and feedback channels.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-06-14
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

typedef enum
{
    ADC_PHASE_SETPOINT = 0,
    ADC_PHASE_FEEDBACK = 1
} adc_phase_t;

static volatile uint16_t adcSetpointCounts;
static volatile uint16_t adcFeedbackCounts;
static volatile bool reportServoSample;

static volatile adc_phase_t adcPhase;

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

static void DAC1_WriteCounts(uint8_t counts)
{
    DAC1DATL = counts;
}

static void Servo_UpdateDac(uint16_t setpointCounts, uint16_t feedbackCounts)
{
    int16_t errorCounts = (int16_t)setpointCounts - (int16_t)feedbackCounts;

    if (errorCounts > (int16_t)SERVO_DEADBAND_COUNTS)
    {
        if (DAC1DATL <= (uint8_t)(DAC_MAX_COUNTS - SERVO_STEP_COUNTS))
        {
            DAC1_WriteCounts((uint8_t)(DAC1DATL + SERVO_STEP_COUNTS));
        }
        else
        {
            DAC1_WriteCounts(DAC_MAX_COUNTS);
        }
    }
    else if (errorCounts < -(int16_t)SERVO_DEADBAND_COUNTS)
    {
        if (DAC1DATL >= SERVO_STEP_COUNTS)
        {
            DAC1_WriteCounts((uint8_t)(DAC1DATL - SERVO_STEP_COUNTS));
        }
        else
        {
            DAC1_WriteCounts(0);
        }
    }
}

#if defined(VECTORED_INTERRUPTS_ENABLED)
/// @brief Handle UART1 receive interrupts.
/// @param None.
/// @return None.
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
{
    UART_HandleRxInterrupt(&console_uart);
}

/// @brief  Handle UART1 transmit interrupts.
/// @param None.
/// @return None.
void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
{
    UART_HandleTxInterrupt(&console_uart);
}

/// @brief  Handle ADC interrupts.
/// @param None.
/// @return None.
void __interrupt(irq(IRQ_AD), high_priority) ADC_ISR(void)
{
    if (PIR1bits.ADIF)
    {
        uint16_t sampleCounts = ADFLTR;

        if (adcPhase == ADC_PHASE_SETPOINT)
        {
            adcSetpointCounts = sampleCounts;
            adcPhase = ADC_PHASE_FEEDBACK;
        }
        else
        {
            adcFeedbackCounts = sampleCounts;
            adcPhase = ADC_PHASE_SETPOINT;
            reportServoSample = true;
        }

        PIR1bits.ADIF = 0;
    }
}
#else
/// @brief  Handle non-vectored interrupts for UART and ADC.
/// @param None.
/// @return None.
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
        uint16_t sampleCounts = ADFLTR;

        if (adcPhase == ADC_PHASE_SETPOINT)
        {
            adcSetpointCounts = sampleCounts;
            adcPhase = ADC_PHASE_FEEDBACK;
        }
        else
        {
            adcFeedbackCounts = sampleCounts;
            adcPhase = ADC_PHASE_SETPOINT;
            reportServoSample = true;
        }

        PIR1bits.ADIF = 0;
    }
}
#endif

/// @brief  Main application entry point.
/// @param None.
void main(void)
{
    uint16_t triggerCountdownMs = 0;
    uint16_t reportCountdownMs = SERVO_REPORT_PERIOD_MS;
    uint8_t dacCounts = (uint8_t)DAC_MV_TO_COUNTS(DAC_STARTUP_MV);

    uint16_t setpointFiltered = 0;
    uint16_t feedbackFiltered = 0;
    bool filterPrimed = false;

    adcSetpointCounts = 0;
    adcFeedbackCounts = 0;
    reportServoSample = false;
    adcPhase = ADC_PHASE_SETPOINT;

    SYSTEM_Initialize();
    DAC1_WriteCounts(dacCounts);

    if (!UART_Open(&console_uart))
    {
        while (1)
        {
        }
    }
    UART_SelectPrintfTarget(&console_uart);

    printf("DAC 02 Software Servo%s", CRLF);
    printf("sample=%u ms, deadband=%u counts, step=%u count%s",
           SERVO_SAMPLE_PERIOD_MS,
           SERVO_DEADBAND_COUNTS,
           SERVO_STEP_COUNTS,
           CRLF);

    while (1)
    {
        if (reportServoSample)
        {
            uint16_t setpointCounts;
            uint16_t feedbackCounts;
            int16_t errorCounts;

            INTCON0bits.GIEH = 0;
            setpointCounts = adcSetpointCounts;
            feedbackCounts = adcFeedbackCounts;
            reportServoSample = false;
            INTCON0bits.GIEH = 1;

            if (!filterPrimed)
            {
                setpointFiltered = setpointCounts;
                feedbackFiltered = feedbackCounts;
                filterPrimed = true;
            }
            else
            {
                setpointFiltered = (uint16_t)(setpointFiltered +
                                              (((int16_t)setpointCounts - (int16_t)setpointFiltered) >> SERVO_FILTER_SHIFT));
                feedbackFiltered = (uint16_t)(feedbackFiltered +
                                              (((int16_t)feedbackCounts - (int16_t)feedbackFiltered) >> SERVO_FILTER_SHIFT));
            }

            Servo_UpdateDac(setpointFiltered, feedbackFiltered);
            dacCounts = DAC1DATL;
            errorCounts = (int16_t)setpointFiltered - (int16_t)feedbackFiltered;

            if (reportCountdownMs == 0)
            {
                uint32_t setpointMv = ADCC_COUNTS_TO_MV(setpointFiltered);
                uint32_t feedbackMv = ADCC_COUNTS_TO_MV(feedbackFiltered);
                uint32_t dacMv = ((uint32_t)dacCounts * APP_VREF_MV + (DAC_MAX_COUNTS / 2)) / DAC_MAX_COUNTS;

                printf("SP:%4lu mV (%4u)  FB:%4lu mV (%4u)  DAC:%4lu mV (%3u)  ERR:%4d%s",
                       setpointMv,
                       setpointFiltered,
                       feedbackMv,
                       feedbackFiltered,
                       dacMv,
                       dacCounts,
                       errorCounts,
                       CRLF);
                reportCountdownMs = SERVO_REPORT_PERIOD_MS;
            }
        }

        if ((triggerCountdownMs == 0) && (ADCON0bits.GO == 0))
        {
            ADPCHbits.PCH = (adcPhase == ADC_PHASE_SETPOINT) ? ADCC_CHANNEL_SETPOINT_AN0 : ADCC_CHANNEL_FEEDBACK_AN1;
            PIR1bits.ADIF = 0;
            ADCON0bits.GO = 1;
            triggerCountdownMs = SERVO_SAMPLE_PERIOD_MS;
        }

        __delay_ms(1);
        if (triggerCountdownMs > 0)
        {
            triggerCountdownMs--;
        }

        if (reportCountdownMs > 0)
        {
            reportCountdownMs--;
        }
    }
}
