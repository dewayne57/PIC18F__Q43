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
#include "mcp23x17.h"
#include "../../Libraries/UARTLIB/uartlib.h"
#include "../../Libraries/DEBUGLIB/debuglib.h"

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

/// @brief Main application entry point.
/// @param  None
/// @return None
void APP_Initialize(void)
{
    printf("UART initialized for console output.");

    // Initialize SPI module with desired settings
    spi_status_t status = SPI_Open(&spi_handle);
    if (status != SPI_SUCCESS)
    {
        printf("Failed to initialize SPI module. Status code: %d\n", status);
        while (1)
            ; // Halt execution if SPI initialization fails
    }
    printf("SPI module initialized successfully.\n");
    // Additional application initialization code can be added here (e.g., initialize peripherals,
    // set up application state, etc.)
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
