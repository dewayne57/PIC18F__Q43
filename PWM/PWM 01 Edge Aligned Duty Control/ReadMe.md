# PWM 01 Edge Aligned Duty Control

## Overview
Planned demonstration for PWM behavior on PIC18F Q43 devices.

## Source Files
- main.c: Program entry point and demo loop.
- config.h: Device configuration bits and shared constants.
- config.c: Baseline GPIO and system initialization.
- pwm.h: Peripheral demo API.
- pwm.c: Peripheral task scaffold for iterative bring-up.

## Status
Source scaffold created. Replace task internals with hardware register configuration next.

## Suggested Milestones
1. Replace software task scaffold with true peripheral register setup.
2. Add interrupt handling and error-path instrumentation where applicable.
3. Add UART or logic-analyzer observable diagnostics.
4. Capture timing and behavior evidence in project notes.
