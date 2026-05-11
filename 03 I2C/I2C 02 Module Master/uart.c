/* *****************************************************************************************
 *   File Name: uart.c
 *   Description: UART1 debug adapter that forwards legacy UART1 APIs to UARTLIB.
 ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "uart.h"

#ifndef _XTAL_FREQ
#define _XTAL_FREQ 64000000UL
#endif

#define UARTLIB_DEBUG_BAUD            115200UL
#define UARTLIB_DEBUG_HIGH_SPEED      1U

#if UARTLIB_DEBUG_HIGH_SPEED
#define UARTLIB_DEBUG_BRG_VALUE       ((_XTAL_FREQ / (4UL * UARTLIB_DEBUG_BAUD)) - 1UL)
#else
#define UARTLIB_DEBUG_BRG_VALUE       ((_XTAL_FREQ / (16UL * UARTLIB_DEBUG_BAUD)) - 1UL)
#endif

#define UARTLIB_TX_BUFFER_SIZE        128U

#include "../../Libraries/UARTLIB/uartlib.h"

static char g_uart1_tx_buffer[UARTLIB_TX_BUFFER_SIZE];

static uart_handle_t g_uart1 = {
    .port = UART_PORT_1,
    .brg_value = (uint16_t)UARTLIB_DEBUG_BRG_VALUE,
    .high_speed_baud = (UARTLIB_DEBUG_HIGH_SPEED != 0U),
    .data_bits = 8U,
    .parity = UART_PARITY_NONE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_control = UART_FLOW_NONE,
    .tx_buffer = g_uart1_tx_buffer,
    .tx_buffer_size = UARTLIB_TX_BUFFER_SIZE,
    .rx_buffer = g_uart1_tx_buffer,
    .rx_buffer_size = UARTLIB_TX_BUFFER_SIZE,
    .tx_head = 0U,
    .tx_tail = 0U,
    .rx_head = 0U,
    .rx_tail = 0U,
    .initialized = false
};

static void UART1_ConfigurePPS(void)
{
    TRISBbits.TRISB0 = 0U;
    TRISBbits.TRISB1 = 1U;

    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0U;

    RB0PPS = 0x20;
    U1RXPPS = 0x09;

    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 1U;
}

void UART1_Initialize(void)
{
    UART1_ConfigurePPS();
    (void)UART_Open(&g_uart1);
    UART_SelectPrintfTarget(&g_uart1);
}

#if UART1_VECTORED_INTERRUPTS
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
#else
void UART1_RX_ISR(void)
#endif
{
    UART_HandleRxInterrupt(&g_uart1);
}

#if UART1_VECTORED_INTERRUPTS
void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
#else
void UART1_TX_ISR(void)
#endif
{
    UART_HandleTxInterrupt(&g_uart1);
}

/* Pull in the shared UARTLIB implementation without copying it per project. */
#include "../../Libraries/UARTLIB/uartlib.c"


