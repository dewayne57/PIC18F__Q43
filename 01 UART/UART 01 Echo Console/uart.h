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

/// @brief Initialize UART1 peripheral state, GPIO mapping, and interrupt settings.
/// @param None
/// @return None
void UART1_Initialize(void);

/// @brief Read one byte from the software RX buffer.
/// @param data Pointer to where the received character is stored.
/// @return true if a byte was read, false if no data is available.
bool UART1_ReadChar(char *data);

/// @brief Get the number of bytes currently buffered in the software RX queue.
/// @param None
/// @return Count of available RX bytes.
uint8_t UART1_RxAvailable(void);

/// @brief UART1 receive interrupt service routine.
/// @param None
/// @return None
void UART1_RX_ISR(void);

/// @brief UART1 transmit interrupt service routine.
/// @param None
/// @return None
void UART1_TX_ISR(void);

#endif /* UART_H */
