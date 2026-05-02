# TMR 06 Sleep Scheduler RTC Tick

## Overview
Use a low-power timer tick to wake from Sleep, run a task, and return to Sleep.

This example demonstrates low-power periodic scheduling. A timer tick wakes
the MCU at fixed intervals. The wake task samples one ADCC channel and reports
a heartbeat plus sampled value over UART 1, then re-enters Sleep.

The demonstration focuses on repeatable wake cadence and reduced active time.

## Status
Planned. Source and schematic capture are next steps.

## Suggested Milestones
1. Select and configure the low-power timer wake source.
2. Implement wake task, sample task, and Sleep return flow.
3. Add UART heartbeat timestamp for wake interval verification.
4. Measure active duty cycle and estimated power savings.