# UART 02 DMA TX Stream

## Overview
Demonstrate DMA-driven UART transmit from double memory buffers with minimal 
CPU overhead.

This demonstration uses a double-buffering (also known as ping-pong buffering) 
approach to allow the application to send data to the UART output (tx) pin.  
When data is initially sent, it is buffered in one of the two free buffers.  
If the data ends with a CR+LF or LF+CR sequence, the buffers are flipped and
the buffer containing data is processed using DMA to transmit the data.  
Concurrently, any additional data is accumulated in the other buffer.  If 
the current buffer becomes full, the buffers are flipped and the process 
repeated as long as DMA operations have been completed.  If the buffers can
not be flipped (one is still being processed by the DMA system and the other
is full or has received a CR+LF/LF+CR sequence), the caller is blocked until 
a buffer becomes available.)

## Source Files
- main.c: Program entry point and demo loop.
- config.h: Device configuration bits and shared constants.
- config.c: Baseline GPIO and system initialization.
- uart_dma_tx.h: DMA demo API.
- uart_dma_tx.c: UART1 + DMA initialization and periodic TX transfer task.

## Test Procedure
1. Build with XC8 and program the target.
2. Observe UART TX pin output and verify repeated "Hello, UART DMA TX Stream!\r\n" frames.
