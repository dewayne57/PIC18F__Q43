/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for UART 01 Interrupt Echo Console.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-04-10
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "uart.h"

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
    char received;
    int counter = 0;

    SYSTEM_Initialize();
    UART1_Initialize();

    while (1)
    {
        // Perform a non-blocking check for received data and echo it back if available. This 
        // allows the main application to remain responsive while still providing UART communication
        // capabilities. The use of UART1_RxAvailable ensures that we only attempt to read when data
        // is present, preventing blocking on an empty buffer.
        while ((UART1_RxAvailable() > 0U)) {
            UART1_ReadChar(&received);
            printf("%c", received);
        }
        
        __delay_ms(1000); 
        printf("Test %i\r\n", counter++);
    }
}

#ifndef UART1_VECTORED_INTERRUPTS
/// @brief Interrupt Service Routine.
/// @note This ISR handles all interrupts for the application.  It does not use the VECTORED 
///       interrupt feature of the PIC18F, so it must call the appropriate handler functions
///       for each peripheral that generates an interrupt.  In this case, it checks if the 
///       UART1 receive or transmit interrupts are enabled and if their respective flags are set,
///       and calls the corresponding handler functions to process the interrupts.  This approach
///       allows for a centralized ISR that can handle multiple interrupt sources without the need
///       for separate vector locations, while still ensuring that each interrupt is processed
///       efficiently and correctly.
/// @param None
/// @return None
/// @note If UART1_VECTORED_INTERRUPTS is set to 1, the UART1 receive and transmit interrupts can be
///       generated as low priority interrupts, and the corresponding handler functions will be
///       called directly from the respective ISRs.  If not set, this main ISR will handle all
///       interrupts, and it is important to ensure that the appropriate handler functions are called
///       for each interrupt source to ensure proper operation of the application.
void __interrupt() ISR(void)
{
    if (PIR4bits.U1RXIF != 0U)
    {
        UART1_RX_ISR();
    }
    if (PIR4bits.U1TXIF != 0U)
    {
        UART1_TX_ISR();
    }

}
#endif