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

void UART1_Initialize(void);
void UART1_Task(void);

#endif /* UART_H */
