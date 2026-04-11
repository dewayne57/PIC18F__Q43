# UART 02 DMA TX Stream

## Overview
Demonstrate DMA-driven UART transmit from memory buffers with minimal CPU overhead.

## Source Files
- main.c: Program entry point and demo loop.
- config.h: Device configuration bits and shared constants.
- config.c: Baseline GPIO and system initialization.
- uart_dma_tx.h: DMA demo API.
- uart_dma_tx.c: UART1 + DMA initialization and periodic TX transfer task.

## Status
Hardware-first implementation added with guarded register blocks for pack portability.

## Suggested Milestones
1. Map exact DMA trigger source for UART1 TX on the target device pack.
2. Add DMA transfer-complete interrupt service routine and status telemetry.
3. Add long-buffer chaining and frame queue support.
4. Measure throughput and CPU load compared to interrupt-driven TX.

## Test Procedure
1. Build with XC8 and program the target.
2. Observe UART TX pin output and verify repeated "UART DMA TX stream" frames.
3. Verify RD0 activity reflects transfer event count progression.
4. Confirm behavior with and without DMA symbols present in the selected DFP.
