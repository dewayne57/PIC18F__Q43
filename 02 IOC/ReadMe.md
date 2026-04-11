# IOC Example Family

This family demonstrates Interrupt-On-Change on PIC18F Q43 devices using both
flat and vectored interrupt architectures.

## What It Is
Interrupt-On-Change (IOC) is a digital input interrupt source that triggers when
selected GPIO pins transition high-to-low or low-to-high.

## What It Does
- Detects configured pin edge events without continuous polling.
- Latches port-level change flags for interrupt service processing.
- Supports low-latency reaction to user input or digital state changes.

## Common Uses
- Pushbutton and keypad event handling.
- Encoder edge detection and user-interface wake events.
- Input-triggered state machines in low-power applications.

## Projects
- IOC 01 Single
- IOC 02 Vectored

## Shared Goals
- Demonstrate IOC setup on a full input port with weak pull-ups.
- Show firmware behavior differences between flat and vectored interrupt models.
- Provide a minimal and reusable interrupt-driven digital input pattern.
