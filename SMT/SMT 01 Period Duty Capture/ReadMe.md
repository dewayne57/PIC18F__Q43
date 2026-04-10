# SMT 01 Period Duty Capture

## Overview
Capture input PWM period and high time, compute duty cycle, and display the
result on PORTD LEDs.

## Device and Pins
- Device: PIC18F27Q43
- Signal input: RB0
- Display output: RD0 to RD7 (duty bucket indication)

## Behavior
- The firmware tracks rising and falling edges of RB0.
- Each complete cycle produces a new period and high-time sample.
- Duty cycle is computed in permil and mapped to 8 LED steps on PORTD.

## Source Files
- main.c: main loop and indicator update logic.
- config.h: configuration bits and project constants.
- config.c: oscillator and GPIO initialization.
- smt.h: capture data model and public API.
- smt.c: capture pipeline scaffold (software tracker with hardware SMT TODO).

## Suggested Milestones
1. Replace software edge tracking with hardware SMT capture mode setup.
2. Add UART output for period/high-time in engineering units.
3. Add overflow and timeout status reporting.
4. Capture scope plots and annotate schematic jumper setup.
