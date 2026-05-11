# TMR 09 CLC Timed Pulse Qualifier

## Overview
Combine timer windows and CLC logic to qualify noisy pulse inputs.

This example creates a timing window with a timer and uses CLC logic to pass
only pulses that occur inside valid timing bounds. Pulses outside the window
are rejected in hardware.

UART 1 reports accepted and rejected pulse counts to demonstrate filtering.

## Status
Planned. Source and schematic capture are next steps.

## Suggested Milestones
1. Configure timer window generation.
2. Route timer and input signal through CLC logic.
3. Count qualified and rejected events.
4. Validate rejection behavior with injected noisy pulses.