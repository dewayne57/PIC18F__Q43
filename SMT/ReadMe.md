# SMT Example Family

This family demonstrates multiple Signal Measurement Timer operating modes on PIC18F Q43.

## Implementation Status Notice
This peripheral family is not yet fully implemented. Current content is planning and
scaffold-focused; register-complete, validated demonstrations are still in progress.

## What It Is
Signal Measurement Timer (SMT) is a dedicated timing peripheral for precise
measurement of period, pulse width, duty cycle, and event timing.

## What It Does
- Captures timing intervals with hardware counters.
- Measures frequency and duty characteristics of incoming digital signals.
- Reduces CPU overhead for high-resolution timing tasks.

## Common Uses
- PWM quality and frequency measurement.
- Pulse-width sensing from ultrasonic, tachometer, or sensor outputs.
- Timestamp capture for event diagnostics.

## Projects
- SMT 01 Period Duty Capture
- SMT 02 Pulse Width Single Shot
- SMT 03 Windowed Frequency Meter
- SMT 04 Signal Quality Monitor
- SMT 05 Sleep Event Timestamping

## Shared Goals
- Show repeatable measurement pipelines.
- Demonstrate overflow, timeout, and jitter handling.
- Provide reusable conversion routines for Hz, us, and duty percent.
