/* *****************************************************************************************
 *   File Name: uart.h
 *   Description: Public API for UART 01 Interrupt Echo Console.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Define the macro UART1_VECTORED_INTERRUPTS if you want the code to use 
 * vectored prioritized interrupts. If undefined, the code uses the single
 * level flat interrupt (legacy) mode. 
 */

void UART1_Initialize(void);
bool UART1_ReadChar(char *data);
uint8_t UART1_RxAvailable(void);
void UART1_RX_ISR(void);
void UART1_TX_ISR(void);

#endif /* UART_H */
