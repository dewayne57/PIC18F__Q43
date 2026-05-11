/* *****************************************************************************************
 *   File Name: main.c
 *   Description: This file contains the main program for the IOC Single project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-09
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
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
    .tx_buffer = console_tx_buffer,
    .tx_buffer_size = sizeof(console_tx_buffer),
    .rx_buffer = console_rx_buffer,
    .rx_buffer_size = sizeof(console_rx_buffer),
    .tx_head = 0U,
    .tx_tail = 0U,
    .rx_head = 0U,
    .rx_tail = 0U,
    .initialized = false};

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

     printf("IOC 02 Vectored\r\n");

     while (1)
     {
          // Main loop
          // Add your application code here
     }
}

void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
{
     UART_HandleTxInterrupt(&console_uart);
}

// In vectored mode, application code owns the UART vector ISRs.
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
{
     UART_HandleRxInterrupt(&console_uart);
}
