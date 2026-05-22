/* *****************************************************************************************
 *   File Name: app.c
 *   Description: Hardware I2C1 module master implementation for PIC18F__Q43.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-19
 *
 *   Copyright (c) 2026, Dewayne Hafenstein.
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 *   This is the demonstration application implementation for the I2C 02 Module Master
 *   example. It initializes the system, sets up UART1 for debug output, and configures
 *   the I2C1 hardware module master interface.
 *
 *   This application uses an MCP23017 I2C I/O expander as a remote slave device for
 *   demonstration purposes. The MCP23017 is a 2-port (16-bit) I/O expander with interrupt
 *   capabilities.  It uses several configuration registers to control its behavior.  Reading
 *   the datasheet for the MCP23017 is recommended.  The MCP23017 is configured in banked 
 *   addressing mode, so Port A registers are locate at contiguous addresses.
 * 
 *   Port A is configured as digital inputs with weak pull-ups and interrupt on change 
 *   enabled.  Port A is connected to an 8-switch DIP switch to ground.  When a switch 
 *   is closed, the pin is driven low.  On this switch, moving a lever down is denoted 
 *   as "on" so we must interpret a low input as "on".
 *
 *   When a change on Port A is detected, the MCP23017 asserts its INT pin, which is 
 *   connected to the PIC's RB2/INT1 pin.  The PIC is configured to trigger an interrupt 
 *   on the rising edge of INT1, which is active high.  The INT1 ISR only flags that the 
 *   MCP23017 needs service; APP_Service performs the I2C read/write using the interrupt-
 *   driven I2C1 module.  Port B is configured as digital outputs and is connected to 8 
 *   LEDs.  The state of the LEDs is updated to match the switch positions and drives 8 
 *   LEDs for visual feedback.
 * 
 *   In addition to the LEDs, diagnostic messages are printed to the console over UART1.  
 *   The console output includes the current configuration of Port A whenever it changes.
 *   The application also performs an initial read of Port A and updates Port B accordingly.
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "app.h"
#include "i2c.h"
#include "mcp23x17.h"
#include "../../Libraries/UARTLIB/uartlib.h"

extern uart_handle_t console_uart;
extern i2c_handle_t i2c_master;
extern uint8_t i2c_buffer[16];

static volatile uint8_t s_last_porta_value = 0x00U;
static volatile uint8_t s_pending_porta_value = 0x00U;
static volatile bool s_porta_value_valid = false;
static volatile bool s_porta_report_pending = false;
static volatile bool s_porta_update_pending = false;

static i2c_status_t MCP23017_Write(i2c_handle_t *handle, uint8_t device_address, uint16_t length)
{
    return I2C_Write(handle, device_address, length);
}

static i2c_status_t MCP23017_Read(i2c_handle_t *handle, uint8_t device_address, uint16_t length)
{
    return I2C_Read(handle, device_address, length);
}

/// @brief UART1 RX ISR (vectored)
/// @param  None
/// @return None
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
{
    // Handle UART1 Receive Interrupt
    if ((PIE4bits.U1RXIE != 0U) && (PIR4bits.U1RXIF != 0U))
    {
        UART_HandleRxInterrupt(&console_uart);
    }
}

/// @brief UART1 TX ISR (vectored)
/// @param  None
/// @return None
void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
{
    // Handle UART1 Transmit Interrupt
    if ((PIE4bits.U1TXIE != 0U) && (PIR4bits.U1TXIF != 0U))
    {
        UART_HandleTxInterrupt(&console_uart);
    }
}

/// @brief Handle external interrupt on RB2 and defer MCP23017 service to the main loop
/// @param  None
/// @return None
/// @note The ISR only records that the MCP23017 interrupt needs service. I2C access is
///       performed in APP_Service so the module driver can use vectored interrupts.
void __interrupt(irq(IRQ_INT1), low_priority) Extern_ISR(void)
{
    s_porta_update_pending = true;

    // Clear the INT1 interrupt flag
    PIR6bits.INT1IF = 0;
}

/// @brief Initialize the MCP23017 I/O expander.
/// The MCP23017 is configured in banked addressing mode so Port A registers are
/// contiguous within one block and Port B registers are contiguous within another.
/// @param handle Pointer to i2c_handle_t structure
/// @return i2c_status_t indicating success or error
static i2c_status_t MCP23017_Initialize(i2c_handle_t *handle)
{
    i2c_status_t status;

    // Enter banked mode (BANK=1) while keeping address auto-increment enabled (SEQOP=0).
    // INTPOL=1 makes INT active high; ODR=0 keeps INT in push-pull output mode.
    i2c_buffer[0] = IOCON;
    i2c_buffer[1] = 0x82;
    status = MCP23017_Write(handle, MCP23017_ADDR, 2U);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    // Configure Port A in one contiguous banked transfer.
    i2c_buffer[0] = BANKED_IODIRA;
        i2c_buffer[1] = 0xFF; // IODIRA: inputs
        i2c_buffer[2] = 0x00; // IPOLA
        i2c_buffer[3] = 0xFF; // GPINTENA
        i2c_buffer[4] = 0x00; // DEFVALA
        i2c_buffer[5] = 0x00; // INTCONA
        i2c_buffer[6] = 0x82; // IOCON
        i2c_buffer[7] = 0xFF; // GPPUA
        i2c_buffer[8] = 0x00; // INTFA (write ignored)
        i2c_buffer[9] = 0x00; // INTCAPA (write ignored)
        i2c_buffer[10] = 0x00; // GPIOA
        status = MCP23017_Write(handle, MCP23017_ADDR, 11U);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    // Configure Port B in one contiguous banked transfer.
    i2c_buffer[0] = BANKED_IODIRB;
        i2c_buffer[1] = 0x00; // IODIRB: outputs
        i2c_buffer[2] = 0x00; // IPOLB
        i2c_buffer[3] = 0x00; // GPINTENB
        i2c_buffer[4] = 0x00; // DEFVALB
        i2c_buffer[5] = 0x00; // INTCONB
        i2c_buffer[6] = 0x82; // IOCON
        i2c_buffer[7] = 0x00; // GPPUB
        i2c_buffer[8] = 0x00; // INTFB (write ignored)
        i2c_buffer[9] = 0x00; // INTCAPB (write ignored)
        i2c_buffer[10] = 0x00; // GPIOB
        status = MCP23017_Write(handle, MCP23017_ADDR, 11U);
    if (status != I2C_SUCCESS)
    {
        return status;
    }

    return status;
}

/// @brief Initialize the application.
/// @param  None
/// @return None
/// @note This function initializes the system, UART1 for debug output, and the I2C1
///       hardware module master interface. It also prints status messages to the console.
void APP_Initialize(void)
{
    printf("I2C 02 Module Master\r\n");
    __delay_ms(500);

    /*
     * Configure RB2 as the INT1 input. The MCP23017 INT pin is active high,
     * so the PIC is set to trigger on the rising edge.
     */
    TRISBbits.TRISB2 = 1;   // RB2 is input
    ANSELBbits.ANSELB2 = 0; // RB2 is digital
    WPUBbits.WPUB2 = 1;     // Weak pull-up enabled on RB2

    // Initialize I2C1 hardware module master for 100kHz bus speed
    if (I2C_Initialize(&i2c_master, 100) != I2C_SUCCESS)
    {
        printf("Error: Failed to initialize I2C\r\n");
        while (1)
        {
            // Halt here if I2C initialization fails
        }
    }

    i2c_master.tx_buffer = i2c_buffer;
    i2c_master.tx_buffer_size = sizeof(i2c_buffer);
    i2c_master.rx_buffer = i2c_buffer;
    i2c_master.rx_buffer_size = sizeof(i2c_buffer);

    printf("Initializing MCP23017 I/O expander\r\n");
    __delay_ms(500);
    i2c_status_t mcp_init_status = MCP23017_Initialize(&i2c_master);
    if (mcp_init_status != I2C_SUCCESS)
    {
        printf("Error: Failed to initialize MCP23017 (status=%u)\r\n", (unsigned)mcp_init_status);
        __delay_ms(500);
        while (1)
        {
            // Halt here if MCP23017 initialization fails
        }
    }

    // Initial transfer so OLATB reflects GPIOA before the first external interrupt.
    uint8_t mcp_port_a_value = 0x00U;
    i2c_status_t status;
    i2c_buffer[0] = BANKED_GPIOA;
    status = MCP23017_Write(&i2c_master, MCP23017_ADDR, 1U);
    if (status == I2C_SUCCESS)
    {
        status = MCP23017_Read(&i2c_master, MCP23017_ADDR, 1U);
        if (status == I2C_SUCCESS)
        {
            mcp_port_a_value = i2c_buffer[0];
        }
    }
    if (status == I2C_SUCCESS)
    {
        s_last_porta_value = mcp_port_a_value;
        s_porta_value_valid = true;
        s_pending_porta_value = mcp_port_a_value;
        s_porta_report_pending = true;

        i2c_buffer[0] = BANKED_OLATB;
        i2c_buffer[1] = (uint8_t)(~mcp_port_a_value);
        (void)MCP23017_Write(&i2c_master, MCP23017_ADDR, 2U);
    }

    // Enable external INT1 only after MCP23017 setup is complete.
    PIR6bits.INT1IF = 0;
    PIE6bits.INT1IE = 1;

    LATDbits.LATD0 = 1U;

    printf("I2C initialized successfully\r\n");
    __delay_ms(500);
}

void APP_Service(void)
{
    uint8_t porta_value = 0x00U;
    bool report_now = false;
    bool update_now = false;

    PIE6bits.INT1IE = 0;
    if (s_porta_report_pending)
    {
        porta_value = s_pending_porta_value;
        s_porta_report_pending = false;
        report_now = true;
    }
    if (s_porta_update_pending)
    {
        s_porta_update_pending = false;
        update_now = true;
    }
    PIE6bits.INT1IE = 1;

    if (update_now)
    {
        uint8_t mcp_port_a_value = 0x00U;
        i2c_status_t status;
        i2c_buffer[0] = BANKED_GPIOA;
        status = MCP23017_Write(&i2c_master, MCP23017_ADDR, 1U);
        if (status == I2C_SUCCESS)
        {
            status = MCP23017_Read(&i2c_master, MCP23017_ADDR, 1U);
            if (status == I2C_SUCCESS)
            {
                mcp_port_a_value = i2c_buffer[0];
            }
        }

        if (status == I2C_SUCCESS)
        {
            if ((!s_porta_value_valid) || (mcp_port_a_value != s_last_porta_value))
            {
                s_last_porta_value = mcp_port_a_value;
                s_porta_value_valid = true;
                s_pending_porta_value = mcp_port_a_value;
                s_porta_report_pending = true;
            }

            i2c_buffer[0] = BANKED_OLATB;
            i2c_buffer[1] = (uint8_t)(~mcp_port_a_value);
            (void)MCP23017_Write(&i2c_master, MCP23017_ADDR, 2U);
        }
    }

    if (report_now)
    {
        printf("Config is now %02X\r\n", porta_value);
        __delay_ms(500);
    }
}
