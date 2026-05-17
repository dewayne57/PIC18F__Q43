/* *****************************************************************************************
 *   File Name: uartlib.c
 *   Description: Interrupt-driven multi-UART ring-buffer library for PIC18F Q43/Q84 devices.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 *
 *   Copyright (c) 2026, Dewayne Hafenstein.
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 *   The library keeps UART configuration and ring-buffer state in application-owned
 *   uart_handle_t structures.  The application chooses which UART peripheral each
 *   handle targets, provides the TX and RX buffers, and passes the handle to the API.
 *   The library configures the hardware and manages the buffers and interrupts based 
 *   on the provided handle configuration.  The application code does not need to
 *   maintain separate driver code or global state for each UART peripheral it uses.  
 *   The application MUST however allocate the buffers to be used and initialize the 
 *   UART handle data structure appropriately.  See the ReadMe.md file for details.
 *
 *   This design allows one project to run multiple UART peripherals at once without
 *   duplicating driver code or maintaining one set of global state per UART module.
 *
 * ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "uartlib.h"

#define UARTLIB_WEAK_ISR __attribute__((weak))

// Forward declarations for user application to register handles
static uart_handle_t *uartlib_open_handles[5] = {0};

/// @brief  Register a UART handle in the library's internal tracking array. This is 
/// called by UART_Open() when a handle is successfully opened. It allows the library's 
/// ISRs to find the handle based on the port number and call the appropriate interrupt 
/// handlers.
/// @param uart The uart handle to be registered.
/// @return None
static void uartlib_register_handle(uart_handle_t *uart) {
    if (!uart) return;
    int idx = (int)uart->port - 1;
    if (idx >= 0 && idx < 5) uartlib_open_handles[idx] = uart;
}

/// @brief Unregister a UART handle from the library's internal tracking array. 
/// This is called by UART_Close() when a handle is closed. It ensures that the 
/// library's ISRs no longer reference the closed handle.
/// @param uart The uart handle to be unregistered.
/// @return None
static void uartlib_unregister_handle(uart_handle_t *uart) {
    if (!uart) return;
    int idx = (int)uart->port - 1;
    if (idx >= 0 && idx < 5 && uartlib_open_handles[idx] == uart) uartlib_open_handles[idx] = 0;
}

// Flat ISR: calls all open handles with flat mode
/// @brief Flat ISR handler for all UARTs. This function should be called from 
/// the application's main ISR when using flat interrupt mode. It iterates through 
/// all registered UART handles and calls the appropriate RX and TX handlers for 
/// those that are open and configured for flat mode.
UARTLIB_WEAK_ISR void UARTLIB_FlatISR(void) {
    for (int i = 0; i < 5; ++i) {
        uart_handle_t *uart = uartlib_open_handles[i];
        if (uart && uart->initialized && uart->isr_mode == UART_ISR_FLAT) {
            UART_HandleRxInterrupt(uart);
            UART_HandleTxInterrupt(uart);
        }
    }
}

// Vectored ISRs: one per UART, only call if handle is open and vectored
UARTLIB_WEAK_ISR void UARTLIB_U1RX_ISR(void) {
    uart_handle_t *uart = uartlib_open_handles[0];
    if (uart && uart->initialized && uart->isr_mode == UART_ISR_VECTORED) UART_HandleRxInterrupt(uart);
}
UARTLIB_WEAK_ISR void UARTLIB_U1TX_ISR(void) {
    uart_handle_t *uart = uartlib_open_handles[0];
    if (uart && uart->initialized && uart->isr_mode == UART_ISR_VECTORED) UART_HandleTxInterrupt(uart);
}
UARTLIB_WEAK_ISR void UARTLIB_U2RX_ISR(void) {
    uart_handle_t *uart = uartlib_open_handles[1];
    if (uart && uart->initialized && uart->isr_mode == UART_ISR_VECTORED) UART_HandleRxInterrupt(uart);
}
UARTLIB_WEAK_ISR void UARTLIB_U2TX_ISR(void) {
    uart_handle_t *uart = uartlib_open_handles[1];
    if (uart && uart->initialized && uart->isr_mode == UART_ISR_VECTORED) UART_HandleTxInterrupt(uart);
}
UARTLIB_WEAK_ISR void UARTLIB_U3RX_ISR(void) {
    uart_handle_t *uart = uartlib_open_handles[2];
    if (uart && uart->initialized && uart->isr_mode == UART_ISR_VECTORED) UART_HandleRxInterrupt(uart);
}
UARTLIB_WEAK_ISR void UARTLIB_U3TX_ISR(void) {
    uart_handle_t *uart = uartlib_open_handles[2];
    if (uart && uart->initialized && uart->isr_mode == UART_ISR_VECTORED) UART_HandleTxInterrupt(uart);
}
UARTLIB_WEAK_ISR void UARTLIB_U4RX_ISR(void) {
    uart_handle_t *uart = uartlib_open_handles[3];
    if (uart && uart->initialized && uart->isr_mode == UART_ISR_VECTORED) UART_HandleRxInterrupt(uart);
}
UARTLIB_WEAK_ISR void UARTLIB_U4TX_ISR(void) {
    uart_handle_t *uart = uartlib_open_handles[3];
    if (uart && uart->initialized && uart->isr_mode == UART_ISR_VECTORED) UART_HandleTxInterrupt(uart);
}
UARTLIB_WEAK_ISR void UARTLIB_U5RX_ISR(void) {
    uart_handle_t *uart = uartlib_open_handles[4];
    if (uart && uart->initialized && uart->isr_mode == UART_ISR_VECTORED) UART_HandleRxInterrupt(uart);
}
UARTLIB_WEAK_ISR void UARTLIB_U5TX_ISR(void) {
    uart_handle_t *uart = uartlib_open_handles[4];
    if (uart && uart->initialized && uart->isr_mode == UART_ISR_VECTORED) UART_HandleTxInterrupt(uart);
}

static void UART_SetBaudRate(const uart_handle_t *uart); 
static uart_handle_t *uart_printf_target = (uart_handle_t *)0;

/// @brief Clear the in-handle diagnostic status message.
/// @param uart The uart handle whose status message is to be cleared.
/// @return None
static void UART_ClearStatusMessage(uart_handle_t *uart)
{
    uint8_t i;

    for (i = 0U; i < UART_STATUS_MESSAGE_MAX_BYTES; i++)
    {
        uart->status_message[i] = '\0';
    }
}

/// @brief Return true when value is a power of 2 and large enough for a ring buffer.
/// @param value The value to be checked.
/// @return True if the value is a power of 2 and large enough for a ring buffer, 
/// false otherwise.
static bool UART_IsPowerOfTwo(uint16_t value)
{
    return ((value >= 2U) && ((value & (value - 1U)) == 0U));
}

/// @brief Return true if the port number is in the supported UART1-UART5 range.
/// @param port The UART port number to be checked.
/// @return True if the port number is valid, false otherwise.
static bool UART_PortIsValid(uart_port_t port)
{
    return ((port >= UART_PORT_1) && (port <= UART_PORT_5));
}

/// @brief Return true if the parity enumeration contains a supported value.
/// @param parity The parity mode to be checked.
/// @return True if the parity mode is valid, false otherwise.
static bool UART_ParityIsValid(uart_parity_t parity)
{
    return ((parity == UART_PARITY_NONE) || (parity == UART_PARITY_ODD) || (parity == UART_PARITY_EVEN));
}

/// @brief Return true if the stop-bit enumeration contains a supported value.
/// @param stop_bits The stop-bit mode to be checked.
/// @return True if the stop-bit mode is valid, false otherwise.
static bool UART_StopBitsAreValid(uart_stop_bits_t stop_bits)
{
    return ((stop_bits == UART_STOP_BITS_1) || (stop_bits == UART_STOP_BITS_1_5) || (stop_bits == UART_STOP_BITS_2));
}

/// @brief Return true if the flow-control enumeration contains a supported value.
/// @param flow_control The flow-control mode to be checked.
/// @return True if the flow-control mode is valid, false otherwise.
static bool UART_FlowControlIsValid(uart_flow_t flow_control)
{
    return ((flow_control == UART_FLOW_NONE) || (flow_control == UART_FLOW_XON_XOFF) || (flow_control == UART_FLOW_RTS_CTS));
}

/// @brief Return true when a PPS pin enum value is supported on this device family.
/// @param pin The PPS pin to be checked.
/// @return True if the pin is a valid PPS pin for this device, false otherwise.
static bool UART_PPSPinIsValid(uart_pps_pin_t pin)
{
    return ((pin >= UART_PPS_PIN_RA0) && (pin <= UART_PPS_PIN_RE2));
}

/// @brief Return the UART TX output function code for a given UART port.
/// @param port The UART port to be checked.
/// @return The PPS function code for the UART TX output of the specified port, 
/// or 0 if the port is invalid.
static uint8_t UART_TxOutputFunctionValue(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1: return 0x20U;
    case UART_PORT_2: return 0x24U;
    case UART_PORT_3: return 0x28U;
    case UART_PORT_4: return 0x2CU;
    case UART_PORT_5: return 0x30U;
    default:          return 0x00U;
    }
}

/// @brief Return the UART RTS output function code for a given UART port.
/// @param port The UART port to be checked.
/// @return The PPS function code for the UART RTS output of the specified port,
/// or 0 if the port is invalid.
static uint8_t UART_RtsOutputFunctionValue(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1: return 0x22U;
    case UART_PORT_2: return 0x26U;
    case UART_PORT_3: return 0x2AU;
    case UART_PORT_4: return 0x2EU;
    case UART_PORT_5: return 0x32U;
    default:          return 0x00U;
    }
}

/// @brief Configure TRIS direction for a selected PPS pin.
/// @param pin The PPS pin to be configured.
/// @param input True to configure the pin as an input, false for output.
/// @return True if the pin direction was successfully configured, false otherwise.
static bool UART_SetPinDirection(uart_pps_pin_t pin, bool input)
{
    uint8_t direction = (input ? 1U : 0U);

    switch (pin)
    {
    case UART_PPS_PIN_RA0: TRISAbits.TRISA0 = direction; return true;
    case UART_PPS_PIN_RA1: TRISAbits.TRISA1 = direction; return true;
    case UART_PPS_PIN_RA2: TRISAbits.TRISA2 = direction; return true;
    case UART_PPS_PIN_RA3: TRISAbits.TRISA3 = direction; return true;
    case UART_PPS_PIN_RA4: TRISAbits.TRISA4 = direction; return true;
    case UART_PPS_PIN_RA5: TRISAbits.TRISA5 = direction; return true;
    case UART_PPS_PIN_RA6: TRISAbits.TRISA6 = direction; return true;
    case UART_PPS_PIN_RA7: TRISAbits.TRISA7 = direction; return true;

    case UART_PPS_PIN_RB0: TRISBbits.TRISB0 = direction; return true;
    case UART_PPS_PIN_RB1: TRISBbits.TRISB1 = direction; return true;
    case UART_PPS_PIN_RB2: TRISBbits.TRISB2 = direction; return true;
    case UART_PPS_PIN_RB3: TRISBbits.TRISB3 = direction; return true;
    case UART_PPS_PIN_RB4: TRISBbits.TRISB4 = direction; return true;
    case UART_PPS_PIN_RB5: TRISBbits.TRISB5 = direction; return true;
    case UART_PPS_PIN_RB6: TRISBbits.TRISB6 = direction; return true;
    case UART_PPS_PIN_RB7: TRISBbits.TRISB7 = direction; return true;

    case UART_PPS_PIN_RC0: TRISCbits.TRISC0 = direction; return true;
    case UART_PPS_PIN_RC1: TRISCbits.TRISC1 = direction; return true;
    case UART_PPS_PIN_RC2: TRISCbits.TRISC2 = direction; return true;
    case UART_PPS_PIN_RC3: TRISCbits.TRISC3 = direction; return true;
    case UART_PPS_PIN_RC4: TRISCbits.TRISC4 = direction; return true;
    case UART_PPS_PIN_RC5: TRISCbits.TRISC5 = direction; return true;
    case UART_PPS_PIN_RC6: TRISCbits.TRISC6 = direction; return true;
    case UART_PPS_PIN_RC7: TRISCbits.TRISC7 = direction; return true;

        case UART_PPS_PIN_RD0:
        case UART_PPS_PIN_RD1:
        case UART_PPS_PIN_RD2:
        case UART_PPS_PIN_RD3:
        case UART_PPS_PIN_RD4:
        case UART_PPS_PIN_RD5:
        case UART_PPS_PIN_RD6:
        case UART_PPS_PIN_RD7:
    #if defined(TRISD)
        switch (pin)
        {
        case UART_PPS_PIN_RD0: TRISDbits.TRISD0 = direction; return true;
        case UART_PPS_PIN_RD1: TRISDbits.TRISD1 = direction; return true;
        case UART_PPS_PIN_RD2: TRISDbits.TRISD2 = direction; return true;
        case UART_PPS_PIN_RD3: TRISDbits.TRISD3 = direction; return true;
        case UART_PPS_PIN_RD4: TRISDbits.TRISD4 = direction; return true;
        case UART_PPS_PIN_RD5: TRISDbits.TRISD5 = direction; return true;
        case UART_PPS_PIN_RD6: TRISDbits.TRISD6 = direction; return true;
        case UART_PPS_PIN_RD7: TRISDbits.TRISD7 = direction; return true;
        default: break;
        }
    #endif
        return false;

        case UART_PPS_PIN_RE0:
        case UART_PPS_PIN_RE1:
        case UART_PPS_PIN_RE2:
        case UART_PPS_PIN_RE3:
    #if defined(TRISE)
        switch (pin)
        {
        case UART_PPS_PIN_RE0: TRISEbits.TRISE0 = direction; return true;
        case UART_PPS_PIN_RE1: TRISEbits.TRISE1 = direction; return true;
        case UART_PPS_PIN_RE2: TRISEbits.TRISE2 = direction; return true;
        default: break;
        }
    #endif
        return false;

    default:
        return false;
    }
}

/// @brief Force a selected pin to digital mode when ANSEL is available.
/// @param pin The PPS pin to be configured as digital.
/// @return True if the pin was successfully configured as digital, false otherwise.
static bool UART_DisableAnalogOnPin(uart_pps_pin_t pin)
{
    switch (pin)
    {
    case UART_PPS_PIN_RA0: ANSELAbits.ANSELA0 = 0U; return true;
    case UART_PPS_PIN_RA1: ANSELAbits.ANSELA1 = 0U; return true;
    case UART_PPS_PIN_RA2: ANSELAbits.ANSELA2 = 0U; return true;
    case UART_PPS_PIN_RA3: ANSELAbits.ANSELA3 = 0U; return true;
    case UART_PPS_PIN_RA4: ANSELAbits.ANSELA4 = 0U; return true;
    case UART_PPS_PIN_RA5: ANSELAbits.ANSELA5 = 0U; return true;
    case UART_PPS_PIN_RA6: ANSELAbits.ANSELA6 = 0U; return true;
    case UART_PPS_PIN_RA7: ANSELAbits.ANSELA7 = 0U; return true;

    case UART_PPS_PIN_RB0: ANSELBbits.ANSELB0 = 0U; return true;
    case UART_PPS_PIN_RB1: ANSELBbits.ANSELB1 = 0U; return true;
    case UART_PPS_PIN_RB2: ANSELBbits.ANSELB2 = 0U; return true;
    case UART_PPS_PIN_RB3: ANSELBbits.ANSELB3 = 0U; return true;
    case UART_PPS_PIN_RB4: ANSELBbits.ANSELB4 = 0U; return true;
    case UART_PPS_PIN_RB5: ANSELBbits.ANSELB5 = 0U; return true;
    case UART_PPS_PIN_RB6: ANSELBbits.ANSELB6 = 0U; return true;
    case UART_PPS_PIN_RB7: ANSELBbits.ANSELB7 = 0U; return true;

    case UART_PPS_PIN_RC0: ANSELCbits.ANSELC0 = 0U; return true;
    case UART_PPS_PIN_RC1: ANSELCbits.ANSELC1 = 0U; return true;
    case UART_PPS_PIN_RC2: ANSELCbits.ANSELC2 = 0U; return true;
    case UART_PPS_PIN_RC3: ANSELCbits.ANSELC3 = 0U; return true;
    case UART_PPS_PIN_RC4: ANSELCbits.ANSELC4 = 0U; return true;
    case UART_PPS_PIN_RC5: ANSELCbits.ANSELC5 = 0U; return true;
    case UART_PPS_PIN_RC6: ANSELCbits.ANSELC6 = 0U; return true;
    case UART_PPS_PIN_RC7: ANSELCbits.ANSELC7 = 0U; return true;

        case UART_PPS_PIN_RD0:
        case UART_PPS_PIN_RD1:
        case UART_PPS_PIN_RD2:
        case UART_PPS_PIN_RD3:
        case UART_PPS_PIN_RD4:
        case UART_PPS_PIN_RD5:
        case UART_PPS_PIN_RD6:
        case UART_PPS_PIN_RD7:
    #if defined(ANSELD)
        switch (pin)
        {
        case UART_PPS_PIN_RD0: ANSELDbits.ANSELD0 = 0U; return true;
        case UART_PPS_PIN_RD1: ANSELDbits.ANSELD1 = 0U; return true;
        case UART_PPS_PIN_RD2: ANSELDbits.ANSELD2 = 0U; return true;
        case UART_PPS_PIN_RD3: ANSELDbits.ANSELD3 = 0U; return true;
        case UART_PPS_PIN_RD4: ANSELDbits.ANSELD4 = 0U; return true;
        case UART_PPS_PIN_RD5: ANSELDbits.ANSELD5 = 0U; return true;
        case UART_PPS_PIN_RD6: ANSELDbits.ANSELD6 = 0U; return true;
        case UART_PPS_PIN_RD7: ANSELDbits.ANSELD7 = 0U; return true;
        default: break;
        }
    #endif
        return false;

        case UART_PPS_PIN_RE0:
        case UART_PPS_PIN_RE1:
        case UART_PPS_PIN_RE2:
        case UART_PPS_PIN_RE3:
    #if defined(ANSELE)
        switch (pin)
        {
        case UART_PPS_PIN_RE0: ANSELEbits.ANSELE0 = 0U; return true;
        case UART_PPS_PIN_RE1: ANSELEbits.ANSELE1 = 0U; return true;
        case UART_PPS_PIN_RE2: ANSELEbits.ANSELE2 = 0U; return true;
        default: break;
        }
    #endif
        return false;

    default:
        return false;
    }
}

/// @brief Assign a peripheral output function code to a selected PPS 
/// output pin register.
/// @param pin The PPS pin to be configured as an output.
/// @param function_value The PPS function code to be assigned to the pin's output register.
/// @return True if the function code was successfully assigned to the pin, false otherwise.
static bool UART_AssignOutputPPS(uart_pps_pin_t pin, uint8_t function_value)
{
    switch (pin)
    {
    case UART_PPS_PIN_RA0:
#if defined(RA0PPS)
        RA0PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RA1:
#if defined(RA1PPS)
        RA1PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RA2:
#if defined(RA2PPS)
        RA2PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RA3:
#if defined(RA3PPS)
        RA3PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RA4:
#if defined(RA4PPS)
        RA4PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RA5:
#if defined(RA5PPS)
        RA5PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RA6:
#if defined(RA6PPS)
        RA6PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RA7:
#if defined(RA7PPS)
        RA7PPS = function_value;
        return true;
#else
        return false;
#endif

    case UART_PPS_PIN_RB0:
#if defined(RB0PPS)
        RB0PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RB1:
#if defined(RB1PPS)
        RB1PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RB2:
#if defined(RB2PPS)
        RB2PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RB3:
#if defined(RB3PPS)
        RB3PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RB4:
#if defined(RB4PPS)
        RB4PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RB5:
#if defined(RB5PPS)
        RB5PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RB6:
#if defined(RB6PPS)
        RB6PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RB7:
#if defined(RB7PPS)
        RB7PPS = function_value;
        return true;
#else
        return false;
#endif

    case UART_PPS_PIN_RC0:
#if defined(RC0PPS)
        RC0PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RC1:
#if defined(RC1PPS)
        RC1PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RC2:
#if defined(RC2PPS)
        RC2PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RC3:
#if defined(RC3PPS)
        RC3PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RC4:
#if defined(RC4PPS)
        RC4PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RC5:
#if defined(RC5PPS)
        RC5PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RC6:
#if defined(RC6PPS)
        RC6PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RC7:
#if defined(RC7PPS)
        RC7PPS = function_value;
        return true;
#else
        return false;
#endif

    case UART_PPS_PIN_RD0:
#if defined(RD0PPS)
        RD0PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RD1:
#if defined(RD1PPS)
        RD1PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RD2:
#if defined(RD2PPS)
        RD2PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RD3:
#if defined(RD3PPS)
        RD3PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RD4:
#if defined(RD4PPS)
        RD4PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RD5:
#if defined(RD5PPS)
        RD5PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RD6:
#if defined(RD6PPS)
        RD6PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RD7:
#if defined(RD7PPS)
        RD7PPS = function_value;
        return true;
#else
        return false;
#endif

    case UART_PPS_PIN_RE0:
#if defined(RE0PPS)
        RE0PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RE1:
#if defined(RE1PPS)
        RE1PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RE2:
#if defined(RE2PPS)
        RE2PPS = function_value;
        return true;
#else
        return false;
#endif
    case UART_PPS_PIN_RE3:
#if defined(RE3PPS)
        RE3PPS = function_value;
        return true;
#else
        return false;
#endif

    default:
        return false;
    }
}

/// @brief Configure UART input PPS registers from selected pin values.
/// @param port The UART port for which to configure the input PPS.
/// @param rx_pin The selected RX pin PPS value.
/// @param cts_pin The selected CTS pin PPS value (if using RTS/CTS flow control).
/// @param use_cts True if RTS/CTS flow control is being used and CTS pin
/// should be configured, false if not using RTS/CTS and CTS pin can be ignored.
/// @return True if the input PPS registers were successfully configured, false otherwise.
static bool UART_AssignInputPPS(uart_port_t port, uart_pps_pin_t rx_pin, uart_pps_pin_t cts_pin, bool use_cts)
{
    uint8_t rx_value = (uint8_t)((uint8_t)rx_pin - 1U);
    uint8_t cts_value = (uint8_t)((uint8_t)cts_pin - 1U);

    switch (port)
    {
    case UART_PORT_1:
        U1RXPPS = rx_value;
        if (use_cts)
        {
            U1CTSPPS = cts_value;
        }
        return true;

    case UART_PORT_2:
        U2RXPPS = rx_value;
        if (use_cts)
        {
            U2CTSPPS = cts_value;
        }
        return true;

    case UART_PORT_3:
        U3RXPPS = rx_value;
        if (use_cts)
        {
            U3CTSPPS = cts_value;
        }
        return true;

    case UART_PORT_4:
        U4RXPPS = rx_value;
        if (use_cts)
        {
            U4CTSPPS = cts_value;
        }
        return true;

    case UART_PORT_5:
        U5RXPPS = rx_value;
        if (use_cts)
        {
            U5CTSPPS = cts_value;
        }
        return true;

    default:
        return false;
    }
}

/// @brief Configure UART PPS and TRIS/ANSEL for TX/RX and optional RTS/CTS pins.
/// @param uart The UART handle containing the desired PPS pin configuration and 
/// flow control settings.
/// @return True if PPS and pin configurations were successfully applied, false otherwise.
static bool UART_ConfigurePPS(uart_handle_t *uart)
{
    bool use_hw_flow = (uart->flow_control == UART_FLOW_RTS_CTS);


    // Only configure TX if present
    if (uart->tx_pin != UART_PPS_PIN_NONE) {
        if (!UART_DisableAnalogOnPin(uart->tx_pin)) {
            (void)UART_SetStatusMessage(uart, "PPS: TX pin not on package");
            return false;
        }
        if (!UART_SetPinDirection(uart->tx_pin, false)) {
            (void)UART_SetStatusMessage(uart, "PPS: TX pin dir unavailable");
            return false;
        }
    }
    // Only configure RX if present
    if (uart->rx_pin != UART_PPS_PIN_NONE) {
        if (!UART_DisableAnalogOnPin(uart->rx_pin)) {
            (void)UART_SetStatusMessage(uart, "PPS: RX pin not on package");
            return false;
        }
        if (!UART_SetPinDirection(uart->rx_pin, true)) {
            (void)UART_SetStatusMessage(uart, "PPS: RX pin dir unavailable");
            return false;
        }
    }
    // Only configure RTS/CTS if flow control and present
    if (use_hw_flow) {
        if (uart->rts_pin != UART_PPS_PIN_NONE) {
            if (!UART_DisableAnalogOnPin(uart->rts_pin)) {
                (void)UART_SetStatusMessage(uart, "PPS: RTS pin not package");
                return false;
            }
            if (!UART_SetPinDirection(uart->rts_pin, false)) {
                (void)UART_SetStatusMessage(uart, "PPS: RTS dir unavailable");
                return false;
            }
        }
        if (uart->cts_pin != UART_PPS_PIN_NONE) {
            if (!UART_DisableAnalogOnPin(uart->cts_pin)) {
                (void)UART_SetStatusMessage(uart, "PPS: CTS pin not package");
                return false;
            }
            if (!UART_SetPinDirection(uart->cts_pin, true)) {
                (void)UART_SetStatusMessage(uart, "PPS: CTS dir unavailable");
                return false;
            }
        }
    }

#if defined(PPSLOCK)
    PPSLOCK = 0x55U;
    PPSLOCK = 0xAAU;
    PPSLOCKbits.PPSLOCKED = 0U;
#endif


    // Only assign PPS if present
    if (uart->tx_pin != UART_PPS_PIN_NONE) {
        if (!UART_AssignOutputPPS(uart->tx_pin, UART_TxOutputFunctionValue(uart->port))) {
            (void)UART_SetStatusMessage(uart, "PPS error: TX output map");
            goto pps_lock_and_fail;
        }
    }
    if (use_hw_flow && uart->rts_pin != UART_PPS_PIN_NONE) {
        if (!UART_AssignOutputPPS(uart->rts_pin, UART_RtsOutputFunctionValue(uart->port))) {
            (void)UART_SetStatusMessage(uart, "PPS error: RTS output map");
            goto pps_lock_and_fail;
        }
    }
    if ((uart->rx_pin != UART_PPS_PIN_NONE) || (use_hw_flow && uart->cts_pin != UART_PPS_PIN_NONE)) {
        if (!UART_AssignInputPPS(uart->port, uart->rx_pin, uart->cts_pin, use_hw_flow)) {
            (void)UART_SetStatusMessage(uart, "PPS error: RX/CTS input map");
            goto pps_lock_and_fail;
        }
    }

#if defined(PPSLOCK)
    PPSLOCK = 0x55U;
    PPSLOCK = 0xAAU;
    PPSLOCKbits.PPSLOCKED = 1U;
#endif

    return true;

pps_lock_and_fail:
#if defined(PPSLOCK)
    PPSLOCK = 0x55U;
    PPSLOCK = 0xAAU;
    PPSLOCKbits.PPSLOCKED = 1U;
#endif
    return false;
}

/// @brief Apply UART1 default pin mappings when new PPS fields are omitted.
/// @param uart The UART handle containing the desired PPS pin configuration.
/// @return None. The uart handle is modified in-place to fill in any missing 
/// PPS pin fields with UART1 defaults.
static void UART_ApplyDefaultPins(uart_handle_t *uart)
{
    if (uart->port == UART_PORT_1)
    {
        if (uart->tx_pin == UART_PPS_PIN_NONE)
        {
            uart->tx_pin = UART_PPS_PIN_RB0;
        }

        if (uart->rx_pin == UART_PPS_PIN_NONE)
        {
            uart->rx_pin = UART_PPS_PIN_RB1;
        }

        if (uart->flow_control == UART_FLOW_RTS_CTS)
        {
            if (uart->rts_pin == UART_PPS_PIN_NONE)
            {
                uart->rts_pin = UART_PPS_PIN_RB2;
            }

            if (uart->cts_pin == UART_PPS_PIN_NONE)
            {
                uart->cts_pin = UART_PPS_PIN_RB3;
            }
        }
    }
}

/// @brief Validate an application-owned UART handle before touching hardware.
/// @param uart The UART handle to be validated.
/// @return True if the handle is valid, false otherwise. If false is returned, 
/// a status message is set indicating the reason for the failure.
static bool UART_HandleIsValid(uart_handle_t *uart)
{
    if (uart == (uart_handle_t *)0)
    {
        return false;
    }

    (void)UART_SetStatusMessage(uart, "");

    if (!UART_PortIsValid(uart->port))
    {
        (void)UART_SetStatusMessage(uart, "invalid UART port");
        return false;
    }

    if (uart->tx_buffer == (char *)0)
    {
        (void)UART_SetStatusMessage(uart, "TX buffer is NULL");
        return false;
    }

    if (uart->rx_buffer == (char *)0)
    {
        (void)UART_SetStatusMessage(uart, "RX buffer is NULL");
        return false;
    }

    if (!UART_IsPowerOfTwo(uart->tx_buffer_size))
    {
        (void)UART_SetStatusMessage(uart, "TX size not power of 2");
        return false;
    }

    if (!UART_IsPowerOfTwo(uart->rx_buffer_size))
    {
        (void)UART_SetStatusMessage(uart, "RX size not power of 2");
        return false;
    }

    if ((uart->data_bits != 8U) && (uart->data_bits != 9U))
    {
        (void)UART_SetStatusMessage(uart, "data bits must be 8 or 9");
        return false;
    }

    if (!UART_ParityIsValid(uart->parity))
    {
        (void)UART_SetStatusMessage(uart, "invalid parity mode");
        return false;
    }

    if (!UART_StopBitsAreValid(uart->stop_bits))
    {
        (void)UART_SetStatusMessage(uart, "invalid stop bits");
        return false;
    }

    if (!UART_FlowControlIsValid(uart->flow_control))
    {
        (void)UART_SetStatusMessage(uart, "invalid flow control");
        return false;
    }

    if ((uart->port != UART_PORT_1) && (uart->flow_control != UART_FLOW_NONE))
    {
        (void)UART_SetStatusMessage(uart, "flow control needs UART1");
        return false;
    }


    // At least one of TX or RX must be present
    if ((uart->tx_pin == UART_PPS_PIN_NONE) && (uart->rx_pin == UART_PPS_PIN_NONE)) {
        (void)UART_SetStatusMessage(uart, "missing TX and RX PPS pin");
        return false;
    }
    if (uart->tx_pin != UART_PPS_PIN_NONE && !UART_PPSPinIsValid(uart->tx_pin)) {
        (void)UART_SetStatusMessage(uart, "invalid TX PPS pin");
        return false;
    }
    if (uart->rx_pin != UART_PPS_PIN_NONE && !UART_PPSPinIsValid(uart->rx_pin)) {
        (void)UART_SetStatusMessage(uart, "invalid RX PPS pin");
        return false;
    }
    if (uart->flow_control == UART_FLOW_RTS_CTS) {
        if ((uart->rts_pin == UART_PPS_PIN_NONE) || (uart->cts_pin == UART_PPS_PIN_NONE)) {
            (void)UART_SetStatusMessage(uart, "missing RTS/CTS PPS pin");
            return false;
        }
        if (!UART_PPSPinIsValid(uart->rts_pin)) {
            (void)UART_SetStatusMessage(uart, "invalid RTS PPS pin");
            return false;
        }
        if (!UART_PPSPinIsValid(uart->cts_pin)) {
            (void)UART_SetStatusMessage(uart, "invalid CTS PPS pin");
            return false;
        }
    }

    (void)UART_SetStatusMessage(uart, "OK");
    return true;
}

/// @brief Return the framing mode value programmed into UxCON0.MODE.

static uint8_t UART_FrameModeValue(const uart_handle_t *uart)
{
    if (uart->data_bits == 9U)
    {
        return 0x2U;
    }

    if (uart->parity == UART_PARITY_ODD)
    {
        return 0x8U;
    }

    if (uart->parity == UART_PARITY_EVEN)
    {
        return 0x9U;
    }

    return 0x0U;
}

/// @brief Return the stop-bit field value programmed into UxCON2.STP.
static uint8_t UART_StopBitFieldValue(const uart_handle_t *uart)
{
    if ((uart->stop_bits == UART_STOP_BITS_1_5) || (uart->stop_bits == UART_STOP_BITS_2))
    {
        return (uint8_t)uart->stop_bits;
    }

    return 0U;
}

/// @brief Reset the selected UART control registers before initialization.
static void UART_ResetHardware(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        U1CON0 = 0x00U;
        U1CON1 = 0x00U;
        U1CON2 = 0x00U;
        break;

    case UART_PORT_2:
        U2CON0 = 0x00U;
        U2CON1 = 0x00U;
        U2CON2 = 0x00U;
        break;

    case UART_PORT_3:
        U3CON0 = 0x00U;
        U3CON1 = 0x00U;
        U3CON2 = 0x00U;
        break;

    case UART_PORT_4:
        U4CON0 = 0x00U;
        U4CON1 = 0x00U;
        U4CON2 = 0x00U;
        break;

    case UART_PORT_5:
        U5CON0 = 0x00U;
        U5CON1 = 0x00U;
        U5CON2 = 0x00U;
        break;

    default:
        break;
    }
}

/// @brief Apply framing, stop-bit, and flow-control settings to the selected UART.
static void UART_ApplyConfiguration(const uart_handle_t *uart)
{
    uint8_t mode_value = UART_FrameModeValue(uart);
    uint8_t stop_value = UART_StopBitFieldValue(uart);
    uint8_t flow_value = (uint8_t)uart->flow_control;
    uint8_t brgs_value = (uart->high_speed_baud ? 1U : 0U);

    switch (uart->port)
    {
    case UART_PORT_1:
        U1CON0bits.MODE = mode_value;
        U1CON0bits.BRGS = brgs_value;
        U1CON2bits.STP = stop_value;
        U1CON2bits.FLO = flow_value;
        U1CON2bits.RUNOVF = 1U;
        break;

    case UART_PORT_2:
        U2CON0bits.MODE = mode_value;
        U2CON0bits.BRGS = brgs_value;
        U2CON2bits.STP = stop_value;
        U2CON2bits.FLO = 0U;
        U2CON2bits.RUNOVF = 1U;
        break;

    case UART_PORT_3:
        U3CON0bits.MODE = mode_value;
        U3CON0bits.BRGS = brgs_value;
        U3CON2bits.STP = stop_value;
        U3CON2bits.FLO = 0U;
        U3CON2bits.RUNOVF = 1U;
        break;

    case UART_PORT_4:
        U4CON0bits.MODE = mode_value;
        U4CON0bits.BRGS = brgs_value;
        U4CON2bits.STP = stop_value;
        U4CON2bits.FLO = 0U;
        U4CON2bits.RUNOVF = 1U;
        break;

    case UART_PORT_5:
        U5CON0bits.MODE = mode_value;
        U5CON0bits.BRGS = brgs_value;
        U5CON2bits.STP = stop_value;
        U5CON2bits.FLO = 0U;
        U5CON2bits.RUNOVF = 1U;
        break;

    default:
        break;
    }

    UART_SetBaudRate(uart);
}

/// @brief Program the baud-rate divisor for the selected UART.
static void UART_SetBaudRate(const uart_handle_t *uart)
{
    uint16_t brg_value;

    /*
     * Compute the baud rate generator value from the requested baud rate and mode.  The 
     * formula is:
     * 
     * - Standard speed (high_speed_baud=false): brg_value = (Fosc / (16 * BaudRate)) - 1
     * - High speed     (high_speed_baud=true):  brg_value = (Fosc /  (4 * BaudRate)) - 1
     */
    if (uart->high_speed_baud)
    {
        brg_value = (uint16_t)((uart->fosc / (4U * uart->baud_rate)) - 1U);
    }
    else
    {
        brg_value = (uint16_t)((uart->fosc / (16U * uart->baud_rate)) - 1U);
    }

    switch (uart->port)
    {
    case UART_PORT_1:
        U1BRG = brg_value;
        break;

    case UART_PORT_2:
        U2BRG = brg_value;
        break;

    case UART_PORT_3:
        U3BRG = brg_value;
        break;

    case UART_PORT_4:
        U4BRG = brg_value;
        break;

    case UART_PORT_5:
        U5BRG = brg_value;
        break;

    default:
        break;
    }
}

/// @brief Clear pending UART error flags before enabling the selected module.
static void UART_ClearErrors(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        U1ERRIR = 0x00U;
        break;

    case UART_PORT_2:
        U2ERRIR = 0x00U;
        break;

    case UART_PORT_3:
        U3ERRIR = 0x00U;
        break;

    case UART_PORT_4:
        U4ERRIR = 0x00U;
        break;

    case UART_PORT_5:
        U5ERRIR = 0x00U;
        break;

    default:
        break;
    }
}

/// @brief Enable the TX shift-register-empty interrupt source for the selected UART.
static void UART_EnableShiftEmptyInterrupt(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        U1ERRIEbits.U1TXMTIE = 1U;
        break;

    case UART_PORT_2:
        U2ERRIEbits.U2TXMTIE = 1U;
        break;

    case UART_PORT_3:
        U3ERRIEbits.U3TXMTIE = 1U;
        break;

    case UART_PORT_4:
        U4ERRIEbits.U4TXMTIE = 1U;
        break;

    case UART_PORT_5:
        U5ERRIEbits.U5TXMTIE = 1U;
        break;

    default:
        break;
    }
}

/// @brief Enable or disable the selected UART receive interrupt.
static void UART_SetRxInterruptEnabled(uart_port_t port, bool enabled)
{
    switch (port)
    {
    case UART_PORT_1:
        PIE4bits.U1RXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_2:
        PIE8bits.U2RXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_3:
        PIE9bits.U3RXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_4:
        PIE12bits.U4RXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_5:
        PIE13bits.U5RXIE = (enabled ? 1U : 0U);
        break;

    default:
        break;
    }
}

/// @brief Enable or disable the selected UART transmit interrupt.
static void UART_SetTxInterruptEnabled(uart_port_t port, bool enabled)
{
    switch (port)
    {
    case UART_PORT_1:
        PIE4bits.U1TXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_2:
        PIE8bits.U2TXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_3:
        PIE9bits.U3TXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_4:
        PIE12bits.U4TXIE = (enabled ? 1U : 0U);
        break;

    case UART_PORT_5:
        PIE13bits.U5TXIE = (enabled ? 1U : 0U);
        break;

    default:
        break;
    }
}

/// @brief Return true when the selected UART receive interrupt is enabled and pending.
static bool UART_RxInterruptIsPending(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        return ((PIE4bits.U1RXIE != 0U) && (PIR4bits.U1RXIF != 0U));

    case UART_PORT_2:
        return ((PIE8bits.U2RXIE != 0U) && (PIR8bits.U2RXIF != 0U));

    case UART_PORT_3:
        return ((PIE9bits.U3RXIE != 0U) && (PIR9bits.U3RXIF != 0U));

    case UART_PORT_4:
        return ((PIE12bits.U4RXIE != 0U) && (PIR12bits.U4RXIF != 0U));

    case UART_PORT_5:
        return ((PIE13bits.U5RXIE != 0U) && (PIR13bits.U5RXIF != 0U));

    default:
        return false;
    }
}

/// @brief Return true when the selected UART transmit interrupt is enabled and pending.
static bool UART_TxInterruptIsPending(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        return ((PIE4bits.U1TXIE != 0U) && (PIR4bits.U1TXIF != 0U));

    case UART_PORT_2:
        return ((PIE8bits.U2TXIE != 0U) && (PIR8bits.U2TXIF != 0U));

    case UART_PORT_3:
        return ((PIE9bits.U3TXIE != 0U) && (PIR9bits.U3TXIF != 0U));

    case UART_PORT_4:
        return ((PIE12bits.U4TXIE != 0U) && (PIR12bits.U4TXIF != 0U));

    case UART_PORT_5:
        return ((PIE13bits.U5TXIE != 0U) && (PIR13bits.U5TXIF != 0U));

    default:
        return false;
    }
}

/// @brief Enable TX and RX circuitry, then turn the selected UART module on.
static void UART_EnableHardware(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        U1CON0bits.TXEN = 1U;
        U1CON0bits.RXEN = 1U;
        U1CON1bits.ON = 1U;
        break;

    case UART_PORT_2:
        U2CON0bits.TXEN = 1U;
        U2CON0bits.RXEN = 1U;
        U2CON1bits.ON = 1U;
        break;

    case UART_PORT_3:
        U3CON0bits.TXEN = 1U;
        U3CON0bits.RXEN = 1U;
        U3CON1bits.ON = 1U;
        break;

    case UART_PORT_4:
        U4CON0bits.TXEN = 1U;
        U4CON0bits.RXEN = 1U;
        U4CON1bits.ON = 1U;
        break;

    case UART_PORT_5:
        U5CON0bits.TXEN = 1U;
        U5CON0bits.RXEN = 1U;
        U5CON1bits.ON = 1U;
        break;

    default:
        break;
    }
}

/// @brief Disable TX/RX circuitry and turn the selected UART module off.
static void UART_DisableHardware(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        U1CON0bits.TXEN = 0U;
        U1CON0bits.RXEN = 0U;
        U1CON1bits.ON = 0U;
        break;

    case UART_PORT_2:
        U2CON0bits.TXEN = 0U;
        U2CON0bits.RXEN = 0U;
        U2CON1bits.ON = 0U;
        break;

    case UART_PORT_3:
        U3CON0bits.TXEN = 0U;
        U3CON0bits.RXEN = 0U;
        U3CON1bits.ON = 0U;
        break;

    case UART_PORT_4:
        U4CON0bits.TXEN = 0U;
        U4CON0bits.RXEN = 0U;
        U4CON1bits.ON = 0U;
        break;

    case UART_PORT_5:
        U5CON0bits.TXEN = 0U;
        U5CON0bits.RXEN = 0U;
        U5CON1bits.ON = 0U;
        break;

    default:
        break;
    }
}

/// @brief Discard a startup glitch byte that may have appeared during line settling.
static void UART_DiscardStartupRxByte(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        (void)U1RXB;
        break;

    case UART_PORT_2:
        (void)U2RXB;
        break;

    case UART_PORT_3:
        (void)U3RXB;
        break;

    case UART_PORT_4:
        (void)U4RXB;
        break;

    case UART_PORT_5:
        (void)U5RXB;
        break;

    default:
        break;
    }
}

/// @brief Read one byte from the selected UART hardware receive buffer.
static uint8_t UART_ReadRxByte(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        return U1RXB;

    case UART_PORT_2:
        return U2RXB;

    case UART_PORT_3:
        return U3RXB;

    case UART_PORT_4:
        return U4RXB;

    case UART_PORT_5:
        return U5RXB;

    default:
        return 0U;
    }
}

/// @brief Write one byte into the selected UART hardware transmit buffer.
static void UART_WriteTxByte(uart_port_t port, uint8_t data)
{
    switch (port)
    {
    case UART_PORT_1:
        U1TXB = data;
        break;

    case UART_PORT_2:
        U2TXB = data;
        break;

    case UART_PORT_3:
        U3TXB = data;
        break;

    case UART_PORT_4:
        U4TXB = data;
        break;

    case UART_PORT_5:
        U5TXB = data;
        break;

    default:
        break;
    }
}

/// @brief Return true when the selected UART hardware TX FIFO is empty.
static bool UART_TxHardwareBufferEmpty(uart_port_t port)
{
    switch (port)
    {
    case UART_PORT_1:
        return (U1FIFObits.TXBE != 0U);

    case UART_PORT_2:
        return (U2FIFObits.TXBE != 0U);

    case UART_PORT_3:
        return (U3FIFObits.TXBE != 0U);

    case UART_PORT_4:
        return (U4FIFObits.TXBE != 0U);

    case UART_PORT_5:
        return (U5FIFObits.TXBE != 0U);

    default:
        return false;
    }
}

/// @brief Return the TX ring-buffer mask used for fast wraparound.
static uint16_t UART_TxBufferMask(const uart_handle_t *uart)
{
    return (uint16_t)(uart->tx_buffer_size - 1U);
}

/// @brief Return the RX ring-buffer mask used for fast wraparound.
static uint16_t UART_RxBufferMask(const uart_handle_t *uart)
{
    return (uint16_t)(uart->rx_buffer_size - 1U);
}

/// @brief Return the number of unused bytes remaining in the selected TX ring buffer.
static uint16_t UART_TxBufferFreeCount(const uart_handle_t *uart)
{
    uint16_t used;

    if (uart->tx_head >= uart->tx_tail)
    {
        used = (uint16_t)(uart->tx_head - uart->tx_tail);
    }
    else
    {
        used = (uint16_t)(uart->tx_buffer_size - (uint16_t)(uart->tx_tail - uart->tx_head));
    }

    return (uint16_t)((uart->tx_buffer_size - 1U) - used);
}

/// @brief Push one byte into the selected TX ring buffer.
static bool UART_TxBufferPush(uart_handle_t *uart, char data)
{
    uint16_t next_head = (uint16_t)((uart->tx_head + 1U) & UART_TxBufferMask(uart));

    if (next_head == uart->tx_tail)
    {
        return false;
    }

    uart->tx_buffer[uart->tx_head] = data;
    uart->tx_head = next_head;
    return true;
}

/// @brief Pop one byte from the selected TX ring buffer.
static bool UART_TxBufferPop(uart_handle_t *uart, char *data)
{
    if (uart->tx_head == uart->tx_tail)
    {
        return false;
    }

    *data = uart->tx_buffer[uart->tx_tail];
    uart->tx_tail = (uint16_t)((uart->tx_tail + 1U) & UART_TxBufferMask(uart));
    return true;
}

/// @brief Return the number of unused bytes remaining in the selected RX ring buffer.
static uint16_t UART_RxBufferFreeCount(const uart_handle_t *uart)
{
    uint16_t used;

    if (uart->rx_head >= uart->rx_tail)
    {
        used = (uint16_t)(uart->rx_head - uart->rx_tail);
    }
    else
    {
        used = (uint16_t)(uart->rx_buffer_size - (uint16_t)(uart->rx_tail - uart->rx_head));
    }

    return (uint16_t)((uart->rx_buffer_size - 1U) - used);
}

/// @brief Push one byte into the selected RX ring buffer.
static bool UART_RxBufferPush(uart_handle_t *uart, char data)
{
    uint16_t next_head;

    if (data == 0x00)
    {
        return true;
    }

    next_head = (uint16_t)((uart->rx_head + 1U) & UART_RxBufferMask(uart));
    if (next_head == uart->rx_tail)
    {
        return false;
    }

    uart->rx_buffer[uart->rx_head] = data;
    uart->rx_head = next_head;
    return true;
}

/// @brief Pop one byte from the selected RX ring buffer.
static bool UART_RxBufferPop(uart_handle_t *uart, char *data)
{
    if (uart->rx_head == uart->rx_tail)
    {
        return false;
    }

    *data = uart->rx_buffer[uart->rx_tail];
    uart->rx_tail = (uint16_t)((uart->rx_tail + 1U) & UART_RxBufferMask(uart));
    return true;
}

/// @brief Load the next queued byte into the hardware transmitter for one UART instance.
static void UART_SendNext(uart_handle_t *uart)
{
    char next;

    if (!UART_TxBufferPop(uart, &next))
    {
        UART_SetTxInterruptEnabled(uart->port, false);
        return;
    }

    UART_WriteTxByte(uart->port, (uint8_t)next);
    UART_SetTxInterruptEnabled(uart->port, true);
}

/// @brief Open one UART instance described by an application-owned handle.
bool UART_Open(uart_handle_t *uart)
{
    if (uart == (uart_handle_t *)0)
    {
        return false;
    }

    if ((uart != (uart_handle_t *)0) && (uart->initialized))
    {
        return true;
    }

    UART_ApplyDefaultPins(uart);

    if (!UART_HandleIsValid(uart))
    {
        return false;
    }

    uart->tx_head = 0U;
    uart->tx_tail = 0U;
    uart->rx_head = 0U;
    uart->rx_tail = 0U;
    UART_ClearStatusMessage(uart);
    uart->initialized = false;

    if (!UART_ConfigurePPS(uart))
    {
        return false;
    }

    UART_ResetHardware(uart->port);
    UART_ApplyConfiguration(uart);
    UART_SetBaudRate(uart);
    UART_ClearErrors(uart->port);
    UART_EnableShiftEmptyInterrupt(uart->port);
    UART_SetTxInterruptEnabled(uart->port, false);
    UART_SetRxInterruptEnabled(uart->port, false);
    UART_EnableHardware(uart->port);

    /* Startup settling delay — avoids __delay_ms() dependency on _XTAL_FREQ. */
    { volatile uint16_t i = 0xFFFFU; while (i-- != 0U) { } }
    UART_DiscardStartupRxByte(uart->port);

    UART_SetRxInterruptEnabled(uart->port, true);
    uart->initialized = true;
    return true;
}

/// @brief Close one UART instance so the application can safely reconfigure and reopen it.
void UART_Close(uart_handle_t *uart)
{
    if (uart == (uart_handle_t *)0)
    {
        return;
    }

    if (!uart->initialized)
    {
        return;
    }

    if (uart_printf_target == uart)
    {
        uart_printf_target = (uart_handle_t *)0;
    }

    uart->initialized = false;

    if (!UART_PortIsValid(uart->port))
    {
        (void)UART_SetStatusMessage(uart, "close: invalid UART port");
        uart->tx_head = 0U;
        uart->tx_tail = 0U;
        uart->rx_head = 0U;
        uart->rx_tail = 0U;
        return;
    }

    UART_SetRxInterruptEnabled(uart->port, false);
    UART_SetTxInterruptEnabled(uart->port, false);
    UART_DisableHardware(uart->port);

    uart->tx_head = 0U;
    uart->tx_tail = 0U;
    uart->rx_head = 0U;
    uart->rx_tail = 0U;
}

/// @brief Queue one character for transmission on the selected UART.
bool UART_WriteChar(uart_handle_t *uart, char data)
{
    if (uart == (uart_handle_t *)0)
    {
        return false;
    }

    if (!uart->initialized)
    {
        (void)UART_SetStatusMessage(uart, "write failed: UART not open");
        return false;
    }

    while (UART_TxBufferFreeCount(uart) == 0U)
    {
    }

    if (!UART_TxBufferPush(uart, data))
    {
        (void)UART_SetStatusMessage(uart, "write failed: TX buffer full");
        return false;
    }

    if (UART_TxHardwareBufferEmpty(uart->port) && (!UART_TxInterruptIsPending(uart->port)))
    {
        UART_SendNext(uart);
    }

    return true;
}

/// @brief Read one character from the selected UART receive buffer.
bool UART_ReadChar(uart_handle_t *uart, char *data)
{
    if (uart == (uart_handle_t *)0)
    {
        return false;
    }

    if (data == (char *)0)
    {
        (void)UART_SetStatusMessage(uart, "read failed: data ptr NULL");
        return false;
    }

    if (!uart->initialized)
    {
        (void)UART_SetStatusMessage(uart, "read failed: UART not open");
        return false;
    }

    if (!UART_RxBufferPop(uart, data))
    {
        (void)UART_SetStatusMessage(uart, "read failed: RX buffer empty");
        return false;
    }

    UART_SetRxInterruptEnabled(uart->port, true);
    return true;
}

/// @brief Return the number of bytes currently buffered in the selected UART receive queue.
uint16_t UART_RxAvailable(const uart_handle_t *uart)
{
    if ((uart == (const uart_handle_t *)0) || (!uart->initialized))
    {
        return 0U;
    }

    if (uart->rx_head >= uart->rx_tail)
    {
        return (uint16_t)(uart->rx_head - uart->rx_tail);
    }

    return (uint16_t)(uart->rx_buffer_size - (uint16_t)(uart->rx_tail - uart->rx_head));
}

/// @brief Store a bounded diagnostic status message in the UART handle.
bool UART_SetStatusMessage(uart_handle_t *uart, const char *message)
{
    uint8_t i = 0U;

    if (uart == (uart_handle_t *)0)
    {
        return false;
    }

    if (message != (const char *)0)
    {
        while ((i < (UART_STATUS_MESSAGE_MAX_BYTES - 1U)) && (message[i] != '\0'))
        {
            uart->status_message[i] = message[i];
            i++;
        }
    }

    while (i < UART_STATUS_MESSAGE_MAX_BYTES)
    {
        uart->status_message[i] = '\0';
        i++;
    }

    return true;
}

/// @brief Return the diagnostic status message stored in the UART handle.
const char *UART_GetStatusMessage(const uart_handle_t *uart)
{
    if (uart == (const uart_handle_t *)0)
    {
        return "";
    }

    return uart->status_message;
}

/// @brief Handle a receive interrupt for one UART instance.
void UART_HandleRxInterrupt(uart_handle_t *uart)
{
    if (uart == (uart_handle_t *)0)
    {
        return;
    }

    if (!uart->initialized)
    {
        (void)UART_SetStatusMessage(uart, "RX ISR ignored: UART not open");
        return;
    }

    if (UART_RxInterruptIsPending(uart->port))
    {
        if (UART_RxBufferFreeCount(uart) == 0U)
        {
            (void)UART_ReadRxByte(uart->port);
            UART_SetRxInterruptEnabled(uart->port, false);
            (void)UART_SetStatusMessage(uart, "RX overflow: interrupt disabled");
        }
        else
        {
            if (!UART_RxBufferPush(uart, (char)UART_ReadRxByte(uart->port)))
            {
                UART_SetRxInterruptEnabled(uart->port, false);
                (void)UART_SetStatusMessage(uart, "RX overflow: push failed");
            }
        }
    }
}

/// @brief Handle a transmit interrupt for one UART instance.
void UART_HandleTxInterrupt(uart_handle_t *uart)
{
    if (uart == (uart_handle_t *)0)
    {
        return;
    }

    if (!uart->initialized)
    {
        (void)UART_SetStatusMessage(uart, "TX ISR ignored: UART not open");
        return;
    }

    if (UART_TxInterruptIsPending(uart->port))
    {
        UART_SendNext(uart);
    }
}

/// @brief Select which UART instance the global printf hook should use.
void UART_SelectPrintfTarget(uart_handle_t *uart)
{
    if ((uart != (uart_handle_t *)0) && (!uart->initialized))
    {
        (void)UART_SetStatusMessage(uart, "printf target not initialized");
        return;
    }

    uart_printf_target = uart;
}

/// @brief XC8 printf retarget hook - routes output through the selected UART instance.
void putch(char data)
{
    if (uart_printf_target != (uart_handle_t *)0)
    {
        (void)UART_WriteChar(uart_printf_target, data);
    }
}

