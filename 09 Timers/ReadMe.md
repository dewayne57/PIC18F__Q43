# Timers Example Family

This family demonstrates timer modes for periodic ticks, counting, and timeout control.

## Implementation Status Notice
This peripheral family is not yet fully implemented. Current content is planning and
scaffold-focused; register-complete, validated demonstrations are still in progress.

## What It Is
Timers are hardware counters driven by internal or external clocks for creating
time bases and measuring intervals.

## What It Does
- Generates periodic interrupts.
- Counts pulses over configured windows.
- Produces one-shot timeouts and delays.
- Triggers deterministic ADCC sampling.
- Supports DMA-based sample logging.
- Builds low-power wake sleep schedules.
- Provides timer-synchronized PWM control.
- Implements multi-rate software task scheduling.

## Common Uses
- System scheduling and heartbeat ticks.
- Debounce and timeout supervision.
- Pulse/event counting applications.

## Projects
- TMR 01 Periodic Interrupt Tick
- TMR 02 External Counter Gate
- TMR 03 One Shot Timeout
- TMR 04 Timer Triggered ADCC Sampler
- TMR 05 Timer ADCC DMA Burst Logger
- TMR 06 Sleep Scheduler RTC Tick
- TMR 07 Pulse Width Measurement Gate
- TMR 08 PWM Ramp Generator
- TMR 09 CLC Timed Pulse Qualifier
- TMR 10 Multi Rate Task Wheel

## Shared Goals
- Demonstrate reliable peripheral initialization.
- Show observable behavior with measurable outputs.
- Provide reusable firmware patterns for future projects.
