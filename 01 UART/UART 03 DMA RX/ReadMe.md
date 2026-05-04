# UART 03 DMA RX

## Overview
Demonstrate DMA-driven UART transmit and receive in one project.

This example keeps the DMA TX stream behavior from UART 02 and adds DMA RX
double-buffering (ping-pong). For RX, DMA fills one of two `rx_buffers` while
firmware processes the other. No separate ready-buffer pool is used.

When a DMA transfer completes, application code checks availability with
UART_DMA_RX_BufferAvailable(), then retrieves a direct pointer from
UART_DMA_RX_GetBuffer(). After processing, UART_DMA_RX_ReleaseBuffer() marks the
buffer free so DMA can reuse it.

UART1 hardware flow control is enabled with RTS on RB2 and CTS on RB3. If both
RX buffers are occupied, RX DMA pauses and resumes once a buffer is released.

TX output uses the retained DMA TX path (`putch`), so periodic status text and
echoed RX payloads are transmitted without polling on every byte.

## Source Files
- main.c: Program entry point and demo loop.
- config.h: Device configuration bits and shared constants.
- config.c: Baseline GPIO, UART, DMA1 (RX), DMA2 (TX), and interrupt initialization.
- uart_dma_tx.h: DMA TX API for `putch` output.
- uart_dma_tx.c: DMA TX buffering and ISR logic.
- uart_dma_rx.h: DMA RX ping-pong API.
- uart_dma_rx.c: DMA RX ISR and buffer management logic.

## Test Procedure
1. Build with XC8 and program the target.
2. Connect a UART host terminal at 115200 baud.
3. Verify periodic "Hello, UART DMA TX + RX DMA!" messages.
4. Send at least 256 bytes from the host.
5. Verify the RX buffer status line and echoed payload are transmitted.
