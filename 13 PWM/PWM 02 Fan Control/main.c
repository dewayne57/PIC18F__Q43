/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main program for the demonstration project. 
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-09
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
#include "pwm.h"
#include "../../Libraries/UARTLIB/uartlib.h"

static char console_tx_buffer[128];
static char console_rx_buffer[128];

uart_handle_t console_uart = {
    .port = UART_PORT_1,
    .high_speed_baud = false,
    .baud_rate = 19200U,
    .fosc = _XTAL_FREQ, // Replace with your actual peripheral clock frequency
    .data_bits = 8U,
    .parity = UART_PARITY_NONE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_control = UART_FLOW_NONE,
     .tx_pin = UART_PPS_PIN_RB0,
     .rx_pin = UART_PPS_PIN_RB1,
     .rts_pin = UART_PPS_PIN_NONE,
     .cts_pin = UART_PPS_PIN_NONE,
     .isr_mode = UART_ISR_FLAT,
    .tx_buffer = console_tx_buffer,
    .tx_buffer_size = sizeof(console_tx_buffer),
    .rx_buffer = console_rx_buffer,
    .rx_buffer_size = sizeof(console_rx_buffer),
    .tx_head = 0U,
    .tx_tail = 0U,
    .rx_head = 0U,
    .rx_tail = 0U,
    .initialized = false};

PWM_Handle pwm_handle;

/// @brief Main entry point of the application.
/// This function initializes the system, sets up the UART for debugging, and 
/// enters an infinite loop.  Diagnostic output to the UART echoing the state 
/// of the switch input is printed whenever an IOC event occurs on the configured 
/// pin.  This diagnostic is generated in the IOC interrupt handler in ioc.c.
/// @param  None
/// @return None
void main(void)
{
     // Initialize the system and UART debug channel.
     SYSTEM_Initialize();
     if (!UART_Open(&console_uart))
     {
          while (1)
          {
               // Halt here if UART initialization fails.
          }
     }
     UART_SelectPrintfTarget(&console_uart);
     printf("PWM 02 Fan Control%s", CRLF);

     PWM_Status pwm_status = PWM_Open(&pwm_handle, 26000U, 0U);
     if (pwm_status != PWM_SUCCESS)
     {
          while (1)
          {
               // Halt here if PWM initialization fails.
          }
     }
     printf("PWM initialized successfully.%s", CRLF);

     while (1)
     {
          for (uint16_t duty_cycle = 0; duty_cycle <= 100; duty_cycle += 10)
          {
               printf("Setting duty cycle to %u%%%s", duty_cycle, CRLF);
               PWM_SetDuty(&pwm_handle, duty_cycle);
               __delay_ms(1000);
               __delay_ms(1000);
               __delay_ms(1000);
               __delay_ms(1000);
               __delay_ms(1000);
          }
          for (uint16_t duty_cycle = 90; duty_cycle > 0; duty_cycle -= 10)
          {
               printf("Setting duty cycle to %u%%%s", duty_cycle, CRLF);
               PWM_SetDuty(&pwm_handle, duty_cycle);
               __delay_ms(1000);
               __delay_ms(1000);
               __delay_ms(1000);
               __delay_ms(1000);
               __delay_ms(1000);
          }
     }
}

#ifdef USE_VECTORED_INTERRUPTS
/// @brief UART1 RX ISR (vectored)
/// @param  None
/// @return None
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
{
     UART_HandleRxInterrupt(&console_uart);
}

/// @brief UART1 TX ISR (vectored)
/// @param  None
/// @return None
void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
{
     UART_HandleTxInterrupt(&console_uart);
}
#else 
void __interrupt() ISR(void)
{
     if (PIR4bits.U1RXIF)
     {
          UART_HandleRxInterrupt(&console_uart);
     }
     if (PIR4bits.U1TXIF)
     {
          UART_HandleTxInterrupt(&console_uart);
     }
}
#endif
