# ZCD Example Family

This family demonstrates zero crossing detection and timing event capture.

## Implementation Status Notice
This peripheral family is not yet fully implemented. Current content is planning and
scaffold-focused; register-complete, validated demonstrations are still in progress.

## What It Is
Zero Crossing Detector (ZCD) is a specialized analog detector that identifies
when an AC waveform crosses near zero volts.

## What It Does
- Detects zero-cross events with hardware timing.
- Generates events/interrupts aligned to AC phase transitions.
- Enables phase-coherent control logic.

## Common Uses
- AC mains phase synchronization.
- Triac or phase-angle control timing.
- Line-frequency measurement and monitoring.

## Projects
- ZCD 01 Zero Crossing Timestamp

## Shared Goals
- Demonstrate reliable peripheral initialization.
- Show observable behavior with measurable outputs.
- Provide reusable firmware patterns for future projects.
