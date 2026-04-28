# UART 03 DMA RX Ring Buffer

## Overview
Demonstrate DMA-driven UART receive into a ring buffer for continuous data capture.

## Source Files
- main.c: Program entry point and demo loop.
- config.h: Device configuration bits and shared constants.
- config.c: Baseline GPIO and system initialization.
- uart_dma_rx.h: DMA demo API.
- uart_dma_rx.c: DMA task scaffold for iterative bring-up.

## Status
Source scaffold created. Replace DMA placeholders with peripheral-specific DMA register setup.

## Suggested Milestones
1. Configure DMA channels, trigger source, source and destination addresses.
2. Add transaction state machine and completion interrupt handling.
3. Add bus/error fault handling paths and timeout recovery.
4. Measure throughput and CPU load compared to non-DMA path.
