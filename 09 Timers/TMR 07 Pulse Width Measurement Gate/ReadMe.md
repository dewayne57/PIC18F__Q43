# TMR 07 Pulse Width Measurement Gate

## Overview
Measure pulse high time using timer gate control.

This example uses timer gate mode so the timer counts only while an external
input is high. The measured count is converted to pulse width in microseconds
and reported over UART 1.

The demonstration highlights hardware-timed measurement without tight polling
loops and with predictable timing accuracy.

## Status
Planned. Source and schematic capture are next steps.

## Suggested Milestones
1. Configure timer clock and gate source input.
2. Capture gate start stop transitions and latch count value.
3. Convert count to time units and send UART diagnostics.
4. Validate measurement error against known pulse widths.