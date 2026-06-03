/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for the demonstration project.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "../../Libraries/UARTLIB/uartlib.h"

#define CRLF "\r\n"
#define FVR_FIXED_VOLTAGE_1_024V 0
#define FVR_FIXED_VOLTAGE_2_048V 1
#define FVR_FIXED_VOLTAGE_4_096V 2
#define FVR_LED_ON() (LATBbits.LATB2 = 0)
#define FVR_LED_OFF() (LATBbits.LATB2 = 1)

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

/// @brief  Enable or disable the FVR module.
/// @param enable  true to enable the FVR, false to disable it.
/// @note When enabling the FVR, this function waits for the ready bit to be set,
/// indicating that the FVR is stable and ready for use. When disabling, it waits
/// for the ready bit to clear, ensuring that the FVR is fully disabled before
/// proceeding. This ensures proper timing and stability when changing the state
/// of the FVR module.
void FVR_Enable(bool enable)
{
    if (enable)
    {
        FVRCONbits.EN = 1; // Enable the FVR module
        while (!FVRCONbits.RDY)
        {
            // Wait for the ready bit to be set, indicating the FVR is now enabled and stable
        }
        FVR_LED_ON();
    }
    else
    {
        FVRCONbits.EN = 0; // Disable the FVR module
        while (FVRCONbits.RDY)
        {
            // Wait for the ready bit to clear, indicating the FVR is now disabled
        }
        FVR_LED_OFF();
    }
}

/// @brief  Initialize the FVR module to support the fixed voltage reference outputs.
/// @Note This function maps the FVR output to the RA2 pin and enables the module.
///       The demonstration project will use the FVR to provide a stable reference voltage
///       to the ADC and DAC for measurement and verification. The FVR is configured to
///       support the three fixed voltage levels (1.024V, 2.048V, and 4.096V) that can be
///       selected at runtime.  By outputting the FVR voltage on a pin, we can directly
///       measure it with a multimeter to verify that the FVR is functioning correctly
///       and providing the expected voltage levels. This is especially useful during
///       development and debugging to ensure that the FVR is functioning correctly and
///       providing the expected voltage levels. The DAC is used to buffer the FVR output,
///       which can help ensure more accurate readings.  This is also required because
///       the FVR value is not mappable directly to a pin. The main loop of the application
///       will cycle through the different FVR voltage levels and print the current output
///       voltage to the console for verification. The use of the FVR in this demonstration
///       project allows us to validate that the internal reference voltages are accurate and
///       stable, which is crucial for reliable ADC and DAC operation.
/// @param None
void FVR_Initialize(void)
{
    FVRCON = 0x00;                               // Reset FVR control register to default state (disabled)
    FVRCONbits.ADFVR = FVR_FIXED_VOLTAGE_1_024V; // Default to 1.024V output
    FVR_Enable(true);                            // Enable the FVR module and wait for it to be ready

    // Configure DAC to effectively buffer the FVR output on RA2 for measurement.
    // The DAC is set to use the FVR as its reference, and the output is enabled
    // on RA2. This allows us to measure the FVR output voltage with a multimeter
    // without loading the FVR directly, which can help ensure more accurate readings.
    DAC1CONbits.EN = 1;
    DAC1CONbits.OE = 2;  // Enable DAC output pin on RA2
    DAC1CONbits.PSS = 2; // FVR as positive reference voltage source
    DAC1CONbits.NSS = 0; // VSS as negative reference voltage source
    DAC1DATL = 255;      // Full-scale output
    __delay_ms(10);      // Short delay to allow DAC output to stabilize
}

/// @brief  Set the FVR module to output a specific fixed voltage.
/// @param voltage  The desired fixed voltage level (FVR_FIXED_VOLTAGE_1_024V,
///                 FVR_FIXED_VOLTAGE_2_048V, or FVR_FIXED_VOLTAGE_4_096V)
/// @return None
void FVR_SetFixedVoltage(uint8_t voltage)
{
    switch (voltage)
    {
    case FVR_FIXED_VOLTAGE_1_024V:
        FVRCONbits.ADFVR = FVR_FIXED_VOLTAGE_1_024V;
        break;
    case FVR_FIXED_VOLTAGE_2_048V:
        FVRCONbits.ADFVR = FVR_FIXED_VOLTAGE_2_048V;
        break;
    case FVR_FIXED_VOLTAGE_4_096V:
        FVRCONbits.ADFVR = FVR_FIXED_VOLTAGE_4_096V;
        break;
    default:
        // Invalid voltage selection; default to 1.024V
        FVRCONbits.ADFVR = FVR_FIXED_VOLTAGE_1_024V;
        break;
    }
    // Wait for the ready bit to be set after changing the voltage level
    while (!FVRCONbits.RDY)
    {
    }
}

/// @brief  Get the current output voltage of the FVR module in millivolts.
/// @param  None
/// @return The current FVR output voltage in millivolts.
uint16_t FVR_GetOutputVoltagemV(void)
{
    switch (FVRCONbits.ADFVR)
    {
    case FVR_FIXED_VOLTAGE_1_024V:
        return 1024;
    case FVR_FIXED_VOLTAGE_2_048V:
        return 2048;
    case FVR_FIXED_VOLTAGE_4_096V:
        return 4096;
    default:
        return 0; // Invalid configuration
    }
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
    SYSTEM_Initialize();
    FVR_Initialize();
    if (!UART_Open(&console_uart))
    {
        while (1)
        {
        }
    }
    UART_SelectPrintfTarget(&console_uart);

    printf("FVR 01 Reference Bring Up%s", CRLF);
    while (1)
    {
        FVR_SetFixedVoltage(FVR_FIXED_VOLTAGE_1_024V);
        printf("FVR Output: %u mV%s", FVR_GetOutputVoltagemV(), CRLF);
        __delay_ms(1000);

        FVR_SetFixedVoltage(FVR_FIXED_VOLTAGE_2_048V);
        printf("FVR Output: %u mV%s", FVR_GetOutputVoltagemV(), CRLF);
        __delay_ms(1000);

        FVR_SetFixedVoltage(FVR_FIXED_VOLTAGE_4_096V);
        printf("FVR Output: %u mV%s", FVR_GetOutputVoltagemV(), CRLF);
        __delay_ms(1000);

        FVR_Enable(false); // Disable the FVR to demonstrate that it can be turned off
        printf("FVR Disabled%s", CRLF);
        __delay_ms(1000);

        FVR_Enable(true); // Re-enable the FVR to demonstrate that it can be turned back on
        printf("FVR Re-enabled%s", CRLF);
    }
}
