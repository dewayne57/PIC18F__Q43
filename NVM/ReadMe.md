# NVM Example Family

This family demonstrates safe nonvolatile memory operations and data integrity checks.

## What It Is
Nonvolatile Memory (NVM) controls internal flash/EEPROM programming sequences
for persistent storage.

## What It Does
- Erases and writes memory rows/words through controlled unlock sequences.
- Preserves data across resets and power cycles.
- Provides readback capability for write verification.

## Common Uses
- Device configuration storage.
- Calibration constants and serial-number data.
- Persistent fault logs and runtime counters.

## Projects
- NVM 01 Flash Row Write Verify

## Shared Goals
- Demonstrate reliable peripheral initialization.
- Show observable behavior with measurable outputs.
- Provide reusable firmware patterns for future projects.
