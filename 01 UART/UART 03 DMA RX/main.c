/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Main application for UART 03 DMA RX ping-pong buffer demo.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-01
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
 *   This demo receives UART data into DMA ping-pong buffers. When a full buffer is
 *   available, firmware retrieves the buffer and echoes it back using UART TX.
 *   This demo keeps the DMA TX stream path and adds DMA RX ping-pong buffering.
 *   RX buffers are retrieved by the application and echoed back over UART TX.
 * ***************************************************************************************** */

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "uart_dma_rx.h"
#include "uart_dma_tx.h"

void main(void)
{
    uint16_t ms_tick = 0U;

    UART_Initialize();

    while (true)
    {
        __delay_ms(1);
        ms_tick++;
        if (ms_tick >= 1000U)
        {
            ms_tick = 0U;
            printf("Hello, UART DMA TX + RX DMA!\r\n");
        }

        if (UART_DMA_RX_BufferAvailable())
        {
            uint16_t length = 0U;
            const uint8_t *buffer = UART_DMA_RX_GetBuffer(&length);

            if (buffer != (const uint8_t *)0)
            {
                uint16_t i;
                printf("RX DMA buffer ready: %u bytes\r\n", length);
                for (i = 0U; i < length; i++)
                {
                    putch(buffer[i]);
                }
                putch('\r');
                putch('\n');

                UART_DMA_RX_ReleaseBuffer();
            }
        }
    }
}
