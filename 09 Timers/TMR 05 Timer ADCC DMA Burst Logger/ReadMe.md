# TMR 05 Timer ADCC DMA Burst Logger

## Overview
Use timer-triggered ADCC conversions with DMA transfers into a RAM buffer.

This example shows how to capture bursts of analog data with low CPU load.
A timer event triggers ADCC conversion at a fixed rate, and DMA stores each
result into a circular buffer.

After each burst window, firmware prints summary data over UART 1, including
buffer depth, min max sample, and average sample.

## Status
Planned. Source and schematic capture are next steps.

## Suggested Milestones
1. Configure timer period and ADCC trigger path.
2. Configure DMA source and destination ring buffer.
3. Add burst complete handling and UART summary output.
4. Validate throughput and dropped-sample behavior at high rates.