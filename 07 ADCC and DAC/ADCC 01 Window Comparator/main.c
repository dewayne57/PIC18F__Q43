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


#define LED_LOW_ON() (LATBbits.LATB2 = 0)
#define LED_LOW_OFF() (LATBbits.LATB2 = 1)
#define LED_HIGH_ON() (LATBbits.LATB3 = 0)
#define LED_HIGH_OFF() (LATBbits.LATB3 = 1)
#define LED_WINDOW_ON() (LATBbits.LATB4 = 0)
#define LED_WINDOW_OFF() (LATBbits.LATB4 = 1)

static bool reportWindowState; 
typedef enum {
    WINDOW_UNKNOWN = 0,
    WINDOW_LOW,
    WINDOW_VALID, 
    WINDOW_HIGH            
} Window_State;

static Window_State windowState; 

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
    .tx_pin = UART_PPS_PIN_RB0,    // TX output pin (set to UART_PPS_PIN_NONE for RX-only)
    .rx_pin = UART_PPS_PIN_RB1,    // RX input pin (set to UART_PPS_PIN_NONE for TX-only)
    .rts_pin = UART_PPS_PIN_NONE,  // RTS output pin (set if using hardware flow control)
    .cts_pin = UART_PPS_PIN_NONE,  // CTS input pin (set if using hardware flow control)
    .isr_mode = UART_ISR_VECTORED, // App owns ISR routing; this marks intended interrupt policy
};

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
#else
/// @brief Non-vectored interrupt handler for PIC16F18855.
//void __interrupt() NonVectoredISR(void)
//{
//    if (PIR4bits.U1RXIF)
//    {
//        UART_HandleRxInterrupt(&console_uart);
//    }
//    if (PIR4bits.U1TXIF)
//    {
//        UART_HandleTxInterrupt(&console_uart);
//    }
//}
#endif

static void trackStateChange(Window_State newState) { 
    if (windowState != newState) { 
        reportWindowState = true; 
    }
    windowState = newState;
}

/// @brief Turns on the "LOW" LED and turns off the other LEDs.
/// @param  None
/// @return None
static void LED_ShowLow(void)
{
    trackStateChange(WINDOW_LOW);
    LED_LOW_ON();
    LED_HIGH_OFF();
    LED_WINDOW_OFF();
}

/// @brief Turns on the "HIGH" LED and turns off the other LEDs.
/// @param  None
/// @return None
static void LED_ShowHigh(void)
{
    trackStateChange(WINDOW_HIGH);
    LED_LOW_OFF();
    LED_HIGH_ON();
    LED_WINDOW_OFF();
}

/// @brief  Turns on the "IN-WINDOW" LED and turns off the other LEDs.
/// @param  None
/// @return None
static void LED_ShowInWindow(void)
{
    trackStateChange(WINDOW_VALID);
    LED_LOW_OFF();
    LED_HIGH_OFF();
    LED_WINDOW_ON();
}

/// @brief ADC interrupt service routine. Determines if the ADC result is below, above, or 
/// within the defined thresholds and updates LEDs accordingly.  
/// @param  None
/// @return None
void __interrupt(irq(IRQ_AD), high_priority) ADC_ISR(void)
{
    if (PIR1bits.ADIF)
    {
        uint16_t adc_result = ADRES;
        uint16_t low_threshold = ADLTH;
        uint16_t high_threshold = ADUTH;

        PIR1bits.ADIF = 0;

        if (adc_result < low_threshold)
        {
            LED_ShowLow();
        }
        else if (adc_result > high_threshold)
        {
            LED_ShowHigh();
        }
        else
        {
            LED_ShowInWindow();
        }
    }
}

/// @brief Main function. Initializes the system and enters an infinite loop.
/// The ADCC runs continuously, and the LED states are updated in the ADC_ISR 
/// based on the ADC results.
/// @param  None
/// @return None    
void main(void)
{
    windowState = WINDOW_UNKNOWN; 
    SYSTEM_Initialize();
    if (!UART_Open(&console_uart))
    {
        while (1)
        {
        }
    }
    UART_SelectPrintfTarget(&console_uart);

    printf("ADCC 01 Window Comparator%s", CRLF);
    while (1)
    {
        if (reportWindowState) { 
            reportWindowState = false; 
            char *state; 
            switch (windowState) { 
                case WINDOW_LOW:
                    state = "LOW";
                    break;
                case WINDOW_VALID:
                    state = "VALID";
                    break;
                case WINDOW_HIGH:
                    state = "HIGH"; 
                    break;
                default:
                    state = "UNKNOWN";
            }
            
            printf("Input voltage is %s%s", state, CRLF);
        }
        /* ADCC runs continuously and LED state is updated in ADC_ISR. */
    }
}
