/* *****************************************************************************************
 *   File Name: uart.h
 *   Description: UART1 debug adapter that forwards to the shared UARTLIB library.
 ***************************************************************************************** */
#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

void UART1_Initialize(void);
void UART1_RX_ISR(void);
void UART1_TX_ISR(void);

#endif /* UART_H */

