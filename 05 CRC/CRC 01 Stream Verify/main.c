/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for CRC Stream Verify.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "../../Libraries/UARTLIB/uartlib.h"

static char console_tx_buffer[64];
static char console_rx_buffer[64];
char message[512];
size_t message_len;

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
#endif

#define CRC_SEED 0xFFFF

/// @brief  Initialize the CRC module to support the CRC-16-CCITT polynomial.
/// @param None
/// @Return None
void CRC_Initialize(void)
{
    CRCCON0 = 0x00;     // Reset the CRC module configuration
    CRCCON0bits.EN = 1; // Enable the CRC module
    CRCCON1 = 0xFF;     // Polynomial and Data length set to 16-bits
    CRCXOR = 0x1021;    // Set the polynomial to 0x1021 for CRC-16-CCITT
    CRCACC = CRC_SEED;  // Initialize the CCITT output to 0xFFFF
}

/// @brief  Process a message through the CRC module.
/// @Note The message is processed in 16-bit chunks.  If the message is an odd length,
/// the last byte is padded with 0x00 to make it 16-bits in length.
/// @param message Pointer to the message buffer.
/// @param message_len Length of the message.
/// @return Calculated CRC value.
uint16_t CRC_Process(char *message, size_t message_len)
{
    CRCCON0bits.EN = 1; // Ensure CRC module is enabled
    CRCACC = CRC_SEED;  // Initialize the CCITT output each time we process a message,
                        // so that the caller doesn't have to worry about resetting it
                        // between messages.
    if (message == 0 || message_len < 2)
    {
        return CRC_SEED; // Return the default for the CCITT polynomial.
    }
    message_len -= 2; // Remove the line endings
    if (message_len % 2 != 0)
    {
        message[message_len] = 0x00;
        message_len++;
    }

    for (size_t i = 0; i < message_len; i += 2)
    {
        CRCDATH = (unsigned char)message[i];
        CRCDATL = (unsigned char)message[i + 1];
        CRCCON0bits.CRCGO = 1;
        while (CRCCON0bits.BUSY)
        {
        }
    }
    return CRCACC;
}

/// @brief Main application entry point.
/// @param  None
/// @return None
/// @note This application initializes the system and UART1, then enters an infinite loop
///       where it continuously checks for received data and echoes it back if available.
///       The use of UART1_RxAvailable ensures that we only attempt to read when data is
///       present, preventing blocking on an empty buffer. The main loop remains responsive,
///       allowing for other tasks to be added in the future while maintaining efficient
///       UART communication.
/// @param
void main(void)
{
    int counter = 0;
    char ch;
    char prev_ch = 0;

    SYSTEM_Initialize();
    CRC_Initialize();
    if (!UART_Open(&console_uart))
    {
        while (1)
        {
        }
    }
    UART_SelectPrintfTarget(&console_uart);

    printf("CRC 01 Stream Verify\r\n");
    printf("Enter any message terminated by CR+LF\r\n");
    while (1)
    {
        if (UART_RxAvailable(&console_uart) > 0)
        {
            if (UART_ReadChar(&console_uart, &ch))
            {
                // Accumulate the character received as part of a larger message which
                // will be processed whenever we encounter a CR+LF or LF+CR sequence.
                // Once the line ending is encountered, process the message through the
                // CRC module and echo the original message and its CRC value to the
                // console.
                if (message_len < sizeof(message))
                {
                    message[message_len] = ch;
                    message_len++;
                    message[message_len] = 0;
                }

                if ((ch == '\n' && prev_ch == '\r') || (ch == '\r' && prev_ch == '\n'))
                {
                    printf("Original Message: \"%s\"\r\n", message);
                    printf("CRC: 0x%04X\r\n", CRC_Process(message, message_len));
                    message_len = 0;
                    prev_ch = 0;
                }

                prev_ch = ch;
            }
        }
    }
}
