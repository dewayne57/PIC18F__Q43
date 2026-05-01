# NCO 01 Static Synthesizer

## Overview
Generate selectable fixed frequencies using NCO increment profiles.

## Device and Pins
- Device: PIC18F27Q43
- Synthesized output: RC0
- Optional measurement loopback: RC0 to SMT input pin on companion project

## Behavior
- Firmware cycles through predefined increment profiles.
- Hardware NCO1 increment registers are updated at runtime.
- RC0 is routed for NCO output demonstration and frequency observation.

## Source Files
- main.c: profile selection and runtime stepping.
- config.h: configuration bits and output mapping.
- config.c: system and GPIO initialization.
- nco.h: NCO control API.
- nco.c: accumulator core and increment management.

## Suggested Milestones
1. Validate each profile against expected output on a frequency counter.
2. Add UART commands for runtime increment control.
3. Add profile dwell control and start-stop command handling.
4. Capture frequency accuracy across multiple setpoints.

## Test Procedure
1. Build with XC8 and program the target.
2. Probe RC0 and verify output frequency changes as profiles advance.
3. Confirm monotonic profile sequence and wrap back to profile 0.
4. Record measured frequency for each increment value.
