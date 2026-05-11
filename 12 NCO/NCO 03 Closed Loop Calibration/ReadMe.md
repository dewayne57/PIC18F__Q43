# NCO 03 Closed Loop Calibration

## Overview
Measure NCO output with SMT and auto-correct increment error.

## Device and Pins
- Device: PIC18F27Q43
- Synthesized output: RC0

## Behavior
- Hardware NCO1 is configured and enabled.
- Firmware applies a simple correction step toward a target increment.
- Measurement estimator is currently a placeholder for future SMT integration.

## Source Files
- main.c: calibration control loop and target tracking logic.
- config.h: configuration bits and project constants.
- config.c: baseline GPIO setup for NCO output.
- nco.h: NCO control API.
- nco.c: hardware NCO register setup and increment writes.

## Status
Source scaffold created with hardware NCO control and closed-loop structure.

## Suggested Milestones
1. Integrate SMT measurement to replace placeholder estimator.
2. Add signed error reporting and convergence telemetry over UART.
3. Tune correction gain and update interval for stable convergence.
4. Capture convergence plots for multiple initial offsets.

## Test Procedure
1. Build with XC8 and program the target.
2. Probe RC0 and verify output remains active while calibration loop runs.
3. Confirm increment value trends toward target via debugger watch or UART output.
4. After SMT integration, verify measured frequency convergence criteria.
