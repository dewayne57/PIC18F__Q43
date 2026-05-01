# TMR 01 Periodic Interrupt Tick

## Overview
Planned demonstration for TIMERS behavior on PIC18F Q43 devices.

## Source Files
- main.c: Program entry point and demo loop.
- config.h: Device configuration bits and shared constants.
- config.c: Baseline GPIO and system initialization.
- timer.h: Peripheral demo API.
- timer.c: Peripheral task scaffold for iterative bring-up.

## Status
Source scaffold created. Replace task internals with hardware register configuration next.

## Suggested Milestones
1. Replace software task scaffold with true peripheral register setup.
2. Add interrupt handling and error-path instrumentation where applicable.
3. Add UART or logic-analyzer observable diagnostics.
4. Capture timing and behavior evidence in project notes.
