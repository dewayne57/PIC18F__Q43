/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Continuous ADCC window comparator demo (LOW/HIGH/IN-WINDOW LEDs).
 *   Author: Dewayne Hafenstein
 *   Date: 2026-06-04
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
#include "config.h"
#include "../../Libraries/UARTLIB/uartlib.h"

static char console_tx_buffer[256];
static char console_rx_buffer[128];
static float systemTemperature = 0.0;

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
    .tx_pin = UART_PPS_PIN_RB0,    // TX output pin (set to UART_PPS_PIN_NONE for RX-only)
    .rx_pin = UART_PPS_PIN_RB1,    // RX input pin (set to UART_PPS_PIN_NONE for TX-only)
    .rts_pin = UART_PPS_PIN_NONE,  // RTS output pin (set if using hardware flow control)
    .cts_pin = UART_PPS_PIN_NONE,  // CTS input pin (set if using hardware flow control)
    .isr_mode = UART_ISR_VECTORED, // App owns ISR routing; this marks intended interrupt policy
};

/// @brief Compute the system temperature based on the filtered ADC result.
/// @param adcc_result The filtered ADC result from the ADCC module.
/// @return None
/// The temperature is calculated using the formula: 
///
///       Temperature (°C) = (ADCCAverage * gain) / 256 + offset
///
/// where gain and offset are defined for the internal temperature sensor and are stored in 
/// the device information area. The gain is in mV/°C and the offset is in °C. The ADCCAverage 
/// is the filtered ADC result from the ADCC module.
void computeSystemTemperature(size_t adcc_result) {

    // The address of a 16-bit calibration value stored in program memory that represents the gain
    // for the high temperature range of the internal temperature sensor. This value is device-specific
    // and must be determined through calibration during device manufacturing.  The value is in
    // millivolts per degree Celsius (mV/°C) and is used to convert the raw ADC reading into a temperature
    // value in degrees Celsius.
    size_t gain = TSHR1;                   // Gain in mV/°C for the internal temperature sensor

    // The address of a 16-bit calibration value stored in program memory that represents the offset
    // for the high temperature range of the internal temperature sensor. This value is device-specific
    // and must be determined through calibration during device manufacturing. The value is in degrees
    // Celsius (°C).
    size_t offset = TSHR3; // Offset in °C for the internal temperature sensor

    systemTemperature = ((float)adcc_result * gain) / 256.0f + (float)offset;
}

#if defined(VECTORED_INTERRUPTS_ENABLED)
/// @brief UART1 RX interrupt vector owned by the application.
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
{
    UART_HandleRxInterrupt(&console_uart);
}

/// @brief UART1 TX interrupt vector owned by the application.
void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
{
    UART_HandleTxInterrupt(&console_uart);
}

/// @brief ADC interrupt service routine. Determines if the ADC result is below, above, or
/// within the defined thresholds and updates LEDs accordingly.
/// @param  None
/// @return None
void __interrupt(irq(IRQ_AD), high_priority) ADC_ISR(void)
{
    if (PIR1bits.ADIF)
    {
        computeSystemTemperature(ADFLTR); // Read the filtered ADC result from the ADCC module
    }
 }
#else
/// @brief Non-vectored interrupt handler for PIC16F18855.
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
        computeSystemTemperature(ADFLTR); // Read the filtered ADC result from the ADCC module
    }
}
#endif

/// @brief Main function. Initializes the system and enters an infinite loop.
/// @param  None
/// @return None    
void main(void)
{
    SYSTEM_Initialize();
    if (!UART_Open(&console_uart))
    {
        while (1)
        {
        }
    }
    UART_SelectPrintfTarget(&console_uart);

    printf("Temp 01 Internal Sensor%s", CRLF);
    while (1)
    {
        __delay_ms(1000);
        printf("Temperature: %.1f °C%s", systemTemperature, CRLF);
    }
}
