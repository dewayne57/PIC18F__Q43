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
#define FVR_FIXED_VOLTAGE_1_024V 1
#define FVR_FIXED_VOLTAGE_2_048V 2
#define FVR_FIXED_VOLTAGE_4_096V 3
#define ADC_CHANNEL_FVR 0x3FU
#define ADC_MAX_COUNTS 4095UL
#define ADC_VDD_MV 5000UL
#define FVR_LED_ON() (LATBbits.LATB2 = 0)
#define FVR_LED_OFF() (LATBbits.LATB2 = 1)

static char console_tx_buffer[256];
static char console_rx_buffer[128];

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

/// @brief Initialize ADCC in basic single-conversion mode.
/// @note This samples the internal FVR channel so measured FVR values can be
///       reported over UART.  This is not technically required for FVR bring-up,
///       but it provides a useful verification.
static void ADCC_Initialize(void)
{
    ADCON0 = 0x00;
    ADCON1 = 0x00;
    ADCON2 = 0x00;
    ADCLK = 0x3F;       // Conservative Tad for stable results across clock settings
    ADACQ = 0x0010;     // Small acquisition time before each conversion
    ADREFbits.PREF = 0; // VDD as positive reference
    ADREFbits.NREF = 0; // VSS as negative reference
    ADPCH = ADC_CHANNEL_FVR;
    ADCON0bits.FM = 1; // Right-justified result
    ADCON0bits.CS = 1; // Use dedicated ADCRC clock
    ADCON0bits.ON = 1; // Enable ADC module
}

/// @brief Take one ADC conversion from the selected channel.
/// @param channel  ADC channel to read.
/// @return Raw ADC result.
static uint16_t ADCC_ReadRaw(uint8_t channel)
{
    ADPCH = channel;
    __delay_us(10);
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO)
    {
    }
    return ADRES;
}

/// @brief Average multiple ADC samples for a stable reading (called Over-Sampling).
/// @param channel  ADC channel to read.
/// @return Averaged ADC result.
static uint16_t ADCC_ReadRawAveraged(uint8_t channel)
{
    uint32_t accumulator = 0;
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        accumulator += ADCC_ReadRaw(channel);
    }

    return (uint16_t)(accumulator / 8U);
}

/// @brief  Read the internal FVR channel and return an averaged ADC result.
/// @param  None
/// @return Averaged ADC result from the internal FVR channel.
static uint16_t ADCC_ReadFVRRawAveraged(void)
{
    return ADCC_ReadRawAveraged(ADC_CHANNEL_FVR);
}

/// @brief Convert a raw ADC reading to millivolts using VDD as reference.
/// @param raw  Raw ADC result to convert.
/// @return The corresponding voltage in millivolts.
static uint16_t ADCC_RawToMillivolts(uint16_t raw)
{
    uint32_t mv = ((uint32_t)raw * ADC_VDD_MV + (ADC_MAX_COUNTS / 2UL)) / ADC_MAX_COUNTS;
    return (uint16_t)mv;
}

/// @brief Delay helper used by the demo loop between FVR setpoints.
/// @param None
/// @return None
static void Wait5Seconds(void)
{
    __delay_ms(1000);
    __delay_ms(1000);
    __delay_ms(1000);
    __delay_ms(1000);
    __delay_ms(1000);
}

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
                           //        while (FVRCONbits.RDY)
                           //        {
                           //            // Wait for the ready bit to clear, indicating the FVR is now disabled
                           //        }
        FVR_LED_OFF();
    }
}

/// @brief  Initialize the FVR module to support the fixed voltage reference outputs.
/// @Note This function enables FVR and configures the default fixed voltage.
///       The main loop cycles through FVR levels and reports sampled internal
///       FVR values from ADCC for verification.
/// @param None
void FVR_Initialize(void)
{
    FVRCON = 0x00;                                // Reset FVR control register to default state (disabled)
    FVRCONbits.ADFVR = FVR_FIXED_VOLTAGE_1_024V;  // ADC FVR default: 1.024V
    FVRCONbits.CDAFVR = FVR_FIXED_VOLTAGE_1_024V; // DAC/Comparator FVR default: 1.024V
    FVR_Enable(true);                             // Enable the FVR module and wait for it to be ready
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
        FVRCONbits.CDAFVR = FVR_FIXED_VOLTAGE_1_024V;
        break;
    case FVR_FIXED_VOLTAGE_2_048V:
        FVRCONbits.ADFVR = FVR_FIXED_VOLTAGE_2_048V;
        FVRCONbits.CDAFVR = FVR_FIXED_VOLTAGE_2_048V;
        break;
    case FVR_FIXED_VOLTAGE_4_096V:
        FVRCONbits.ADFVR = FVR_FIXED_VOLTAGE_4_096V;
        FVRCONbits.CDAFVR = FVR_FIXED_VOLTAGE_4_096V;
        break;
    default:
        // Invalid voltage selection; default to 1.024V
        FVRCONbits.ADFVR = FVR_FIXED_VOLTAGE_1_024V;
        FVRCONbits.CDAFVR = FVR_FIXED_VOLTAGE_1_024V;
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
    ADCC_Initialize();
    FVR_Initialize();
    if (!UART_Open(&console_uart))
    {
        while (1)
        {
        }
    }
    UART_SelectPrintfTarget(&console_uart);

    printf("FVR 01 Reference Bring Up%s", CRLF);
    printf("Internal FVR sampled with ADCC (VDD ref = %u mV)%s", (uint16_t)ADC_VDD_MV, CRLF);
    printf("Using ADPCH=0x%02X for FVR internal sample%s", ADC_CHANNEL_FVR, CRLF);
    while (1)
    {
        uint16_t raw_fvr;
        uint16_t measured_fvr_mV;

        FVR_SetFixedVoltage(FVR_FIXED_VOLTAGE_1_024V);
        raw_fvr = ADCC_ReadFVRRawAveraged();
        measured_fvr_mV = ADCC_RawToMillivolts(raw_fvr);
        printf("FVR=%u mV, ADCC=%u mV (raw=%u)%s",
               FVR_GetOutputVoltagemV(), measured_fvr_mV, raw_fvr, CRLF);
        Wait5Seconds();

        FVR_SetFixedVoltage(FVR_FIXED_VOLTAGE_2_048V);
        raw_fvr = ADCC_ReadFVRRawAveraged();
        measured_fvr_mV = ADCC_RawToMillivolts(raw_fvr);
        printf("FVR=%u mV, ADCC=%u mV (raw=%u)%s",
               FVR_GetOutputVoltagemV(), measured_fvr_mV, raw_fvr, CRLF);
        Wait5Seconds();

        FVR_SetFixedVoltage(FVR_FIXED_VOLTAGE_4_096V);
        raw_fvr = ADCC_ReadFVRRawAveraged();
        measured_fvr_mV = ADCC_RawToMillivolts(raw_fvr);
        printf("FVR=%u mV, ADCC=%u mV (raw=%u)%s",
               FVR_GetOutputVoltagemV(), measured_fvr_mV, raw_fvr, CRLF);
        Wait5Seconds();
        
        FVR_Enable(false); 
        printf("FVR has been disabled%s", CRLF); 
        Wait5Seconds(); 
        FVR_Enable(true); 
    }
}
