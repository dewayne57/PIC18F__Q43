# TMR 04 Timer Triggered ADCC Sampler

## Overview
Use a timer period event to trigger ADCC sampling at a fixed rate.

This example demonstrates deterministic analog sampling where the timer,
not firmware loop timing, defines sample cadence. A potentiometer between
+5 V and GND feeds one ADCC channel.

Each timer period starts one conversion. The firmware reports sampled
voltage over UART 1 to show stable sampling intervals and repeatable data.

## Status
Planned. Source and schematic capture are next steps.

## Suggested Milestones
1. Configure one timer as a periodic trigger source.
2. Configure ADCC for trigger-driven conversion and result handling.
3. Add UART output with sample index and voltage.
4. Validate sample interval on a logic analyzer or timestamp output.