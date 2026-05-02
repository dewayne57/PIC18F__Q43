# TMR 08 PWM Ramp Generator

## Overview
Use timer-synchronized PWM to generate a controlled duty-cycle ramp.

This example configures a PWM module with a timer time base and increments
duty cycle from minimum to maximum, then back down in a repeating pattern.

UART 1 reports the current duty command, and the PWM output can be observed
on a scope for period stability and linear duty change.

## Status
Planned. Source and schematic capture are next steps.

## Suggested Milestones
1. Configure timer and PWM period.
2. Implement up down duty ramp state machine.
3. Add UART duty report and clamp bounds.
4. Validate PWM frequency and duty linearity on scope.