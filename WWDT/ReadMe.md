# WWDT Example Family

This family demonstrates windowed watchdog timing constraints and fault behavior.

## What It Is
Windowed Watchdog Timer (WWDT) is a safety timer that requires service only
within a valid time window.

## What It Does
- Detects stalled or runaway software when service is missed.
- Detects early service when code timing is unexpectedly short.
- Forces reset on watchdog violations.

## Common Uses
- Functional safety supervision.
- Detection of task scheduler failures.
- Recovery from lockups in unattended systems.

## Projects
- WWDT 01 Window Service Monitor

## Shared Goals
- Demonstrate reliable peripheral initialization.
- Show observable behavior with measurable outputs.
- Provide reusable firmware patterns for future projects.
