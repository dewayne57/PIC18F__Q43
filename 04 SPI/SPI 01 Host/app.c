/* *****************************************************************************************
 *   File Name: app.c
 *   Description: Contains the application code for the SPI module host demonstration.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-09
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
 ***************************************************************************************** */
#include <xc.h>
#include <stdio.h>
#include <stdbool.h>
#include "config.h"
#include "app.h"
#include "spi.h"
#include "../../common/mcp23x17.h"
#include "../../Libraries/UARTLIB/uartlib.h"
#include "../../Libraries/DEBUGLIB/debuglib.h"

static uint8_t spi_buffer[128];
extern uart_handle_t console_uart; // UART handle for console output

spi_handle_t spi_handle = {
    .clock_speed_khz = 1000,                   // 1 MHz SPI clock speed
    .clock_source = SPI_CLOCK_SOURCE_FOSC,     // Use system clock as SPI clock source
    .bit_order = MSB_FIRST,                    // Transmit most significant bit first
    .mode = SPI_HOST_MODE,                     // Operate as SPI host (master)
    .transfer_mode = SPI_TRANSFER_FULL_DUPLEX, // Full-duplex data exchange
    .clock_polarity = SPI_ACTIVE_LOW,          // Clock idle state is low
    .ss_polarity = SPI_ACTIVE_LOW,             // Slave select active low
    .input_polarity = SPI_ACTIVE_LOW,          // Data input active low
    .output_polarity = SPI_ACTIVE_LOW,         // Data output active low
    .initialized = false                       // SPI module is not initialized yet
};

/// @brief UART1 RX ISR (vectored)
/// @param  None
/// @return None
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
{
    UART_HandleRxInterrupt(&console_uart);
}

/// @brief UART1 TX ISR (vectored)
/// @param  None
/// @return None
void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
{
    UART_HandleTxInterrupt(&console_uart);
}

/// @brief Handle external interrupt on RB2 to read MCP23S17 port A and copy to MCP23S17 port B
/// @param  None
/// @return None
/// @note This function reads the MCP23S17 port A register and writes the value to MCP23S17
///       port B using SPI
void __interrupt(irq(IRQ_INT1), low_priority) Extern_ISR(void)
{
    spi_buffer[0] = MCP23S17_ADDR | 0x01; // Set the device address for subsequent register reads (read bit set)
    spi_buffer[1] = BANKED_GPIOA;         // GPIOA register address
    spi_buffer[2] = 0x00;                 // Dummy byte for reading data

    uint8_t mcp_port_a_value = 0x00;
    spi_status_t status = SPI_Read(&spi_handle, MCP23S17_ADDR, &mcp_port_a_value, 1);
    if (status == SPI_SUCCESS)
    {
        // Write the value read from MCP23S17 port A to MCP23S17 port B
        spi_buffer[0] = MCP23S17_ADDR;    // Set the device address for subsequent register write
        spi_buffer[1] = BANKED_GPIOB;     // GPIOB register address
        spi_buffer[2] = mcp_port_a_value; // Data to write to GPIOB
        status = SPI_Write(&spi_handle, MCP23S17_ADDR, spi_buffer, 3);
    }
}

/// @brief Initialize the MCP23S17 at device id 0 to operate in banked mode, set up
/// port A as weak pull up digital inputs with interrupt on change enabled, and
/// port B as digital outputs.
/// @param  None
/// @return None
void MCP23S17_Initialize(void)
{
    LATCbits.LATC7 = 1;            // release i/o extender from reset
    __delay_ms(10);                // wait to settle
    spi_buffer[0] = MCP23S17_ADDR; // Set the device address for subsequent register writes
    spi_buffer[1] = IOCON;         // IOCON register address
    spi_buffer[2] = 0x82;          // Set IOCON register to enable banked mode
    spi_status_t status = SPI_Write(&spi_handle, MCP23S17_ADDR, spi_buffer, 3);

    if (status != SPI_SUCCESS)
    {
        printf("Failed to initialize MCP23S17. Status code: %d\n\r", status);
        while (1)
            ; // Halt execution if MCP23S17 initialization fails
    }
    status = SPI_WaitForCompletion(&spi_handle);
    if (status != SPI_SUCCESS)
    {
        printf("Failed to wait for MCP23S17 initialization. Status code: %d\n\r", status);
        while (1)
            ; // Halt execution if MCP23S17 initialization fails
    }

    spi_buffer[0] = MCP23S17_ADDR; // Set the device address for subsequent register writes
    spi_buffer[1] = BANKED_IODIRA;
    spi_buffer[2] = 0xFF;  // IODIRA: inputs
    spi_buffer[3] = 0x00;  // IPOLA
    spi_buffer[4] = 0xFF;  // GPINTENA
    spi_buffer[5] = 0x00;  // DEFVALA
    spi_buffer[6] = 0x00;  // INTCONA
    spi_buffer[7] = 0x82;  // IOCON
    spi_buffer[8] = 0xFF;  // GPPUA
    spi_buffer[9] = 0x00;  // INTFA (write ignored)
    spi_buffer[10] = 0x00; // INTCAPA (write ignored)
    spi_buffer[11] = 0x00; // GPIOA
    status = SPI_Write(&spi_handle, MCP23S17_ADDR, spi_buffer, 12);
    if (status != SPI_SUCCESS)
    {
        printf("Failed to initialize MCP23S17 port A. Status code: %d\n\r", status);
        while (1)
            ; // Halt execution if MCP23S17 initialization fails
    }

    status = SPI_WaitForCompletion(&spi_handle);
    if (status != SPI_SUCCESS)
    {
        printf("Failed to wait for MCP23S17 initialization. Status code: %d\n\r", status);
        while (1)
            ; // Halt execution if MCP23S17 initialization fails
    }

    spi_buffer[0] = MCP23S17_ADDR; // Set the device address for subsequent register writes
    spi_buffer[1] = BANKED_IODIRB;
    spi_buffer[2] = 0x00;  // IODIRB: outputs
    spi_buffer[3] = 0x00;  // IPOLB
    spi_buffer[4] = 0x00;  // GPINTENB
    spi_buffer[5] = 0x00;  // DEFVALB
    spi_buffer[6] = 0x00;  // INTCONB
    spi_buffer[7] = 0x82;  // IOCON
    spi_buffer[8] = 0x00;  // GPPUB
    spi_buffer[9] = 0x00;  // INTFB (write ignored)
    spi_buffer[10] = 0x00; // INTCAPB (write ignored)
    spi_buffer[11] = 0x00; // GPIOB
    status = SPI_Write(&spi_handle, MCP23S17_ADDR, spi_buffer, 12);
    if (status != SPI_SUCCESS)
    {
        printf("Failed to initialize MCP23S17 port B. Status code: %d\n\r", status);
        while (1)
            ; // Halt execution if MCP23S17 initialization fails
    }

    status = SPI_WaitForCompletion(&spi_handle);
    if (status != SPI_SUCCESS)
    {
        printf("Failed to wait for MCP23S17 initialization. Status code: %d\n\r", status);
        while (1)
            ; // Halt execution if MCP23S17 initialization fails
    }
}

/// @brief Main application entry point.
/// @param  None
/// @return None
void APP_Initialize(void)
{
    printf("UART initialized for console output.\n\r");

    // Initialize SPI module with desired settings
    spi_status_t status = SPI_Open(&spi_handle);
    if (status != SPI_SUCCESS)
    {
        printf("Failed to initialize SPI module. Status code: %d\n\r", status);
        while (1)
            ; // Halt execution if SPI initialization fails
    }
    printf("SPI module initialized successfully.\n\r");

    printf("Initializing IO Expander\n\r");
    MCP23S17_Initialize();
    printf("IO Expander initialized successfully.\n\r");
}

/// @brief Main application service loop.  This function is called repeatedly from the main loop
///        and should contain the main logic of the application, such as handling SPI transactions,
///        processing received data, updating application state, etc.
/// @param None
/// @return None
void APP_Service(void)
{
    // Main application logic goes here.  For example, you could implement a simple SPI transaction
    // to read from or write to an SPI slave device, process received data, etc.
    // This function will be called repeatedly from the main loop, so it should be designed to
    // run quickly and return control to the main loop to allow for responsive handling of SPI
    // transactions and other events.
}
