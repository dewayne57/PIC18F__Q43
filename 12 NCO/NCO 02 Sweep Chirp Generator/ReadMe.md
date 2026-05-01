# NCO 02 Sweep Chirp Generator

## Overview
Update increment in time to create linear and logarithmic frequency sweeps.

## Device and Pins
- Device: PIC18F27Q43
- Synthesized output: RC0

## Behavior
- Hardware NCO1 is configured and enabled.
- Firmware sweeps increment upward to a ceiling, then back down to a floor.
- RC0 frequency follows the programmed sweep profile.

## Source Files
- main.c: sweep profile state machine.
- config.h: configuration bits and project constants.
- config.c: baseline GPIO setup for NCO output.
- nco.h: NCO control API.
- nco.c: hardware NCO register setup and increment writes.

## Status
Source scaffold created with hardware NCO register flow.

## Suggested Milestones
1. Add selectable sweep shape (linear versus logarithmic).
2. Add UART runtime control for start, stop, and sweep bounds.
3. Capture sweep timing and edge continuity on scope.
4. Correlate measured output with configured increment ramp.

## Test Procedure
1. Build with XC8 and program the target.
2. Probe RC0 and verify frequency ramps up then ramps down repeatedly.
3. Confirm no output discontinuities at direction reversal points.
4. Record start and stop frequencies for sweep bounds.
