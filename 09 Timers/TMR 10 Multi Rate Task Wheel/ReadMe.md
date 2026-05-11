# TMR 10 Multi Rate Task Wheel

## Overview
Build a multi-rate scheduler from one base timer tick.

This example uses a 1 ms periodic timer interrupt as the system time base.
Software task slots run at 250 ms, 1000 ms, and 10,000 ms rates from that single
tick source.

UART 1 reports task execution counters so relative rates and jitter can be
observed during runtime.

## Status
Planned. Source and schematic capture are next steps.

## Suggested Milestones
1. Configure a stable 1 ms timer interrupt.
2. Implement 10 ms, 100 ms, and 1000 ms task slots.
3. Add execution counters and UART diagnostics.
4. Validate drift and jitter under added workload.