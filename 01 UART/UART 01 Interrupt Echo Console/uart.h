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

#ifndef UART1_VECTORED_INTERRUPTS
#define UART1_VECTORED_INTERRUPTS 0
#endif

void UART1_Initialize(void);
bool UART1_ReadChar(char *data);
uint8_t UART1_RxAvailable(void);

#endif /* UART_H */
