# UART Example Family

This family demonstrates UART communication patterns including interrupt driven receive.

## Implementation Status Notice
This peripheral family is not yet fully implemented. Current content is planning and
scaffold-focused; register-complete, validated demonstrations are still in progress.

## What It Is
Universal Asynchronous Receiver Transmitter (UART) is a serial communication
peripheral for byte-oriented asynchronous data exchange.

## What It Does
- Transmits and receives framed serial bytes.
- Supports interrupt-driven receive and buffered TX workflows.
- Provides a simple text/binary host interface.

## Common Uses
- Debug consoles and command shells.
- Device configuration and telemetry links.
- Bootloader and service-port communication.
- High-rate streaming with DMA offload.

## Projects
- UART 01 Interrupt Echo Console
- UART 02 DMA TX Stream
- UART 03 DMA RX Ring Buffer

## Shared Goals
- Demonstrate reliable peripheral initialization.
- Show observable behavior with measurable outputs.
- Provide reusable firmware patterns for future projects.
- Demonstrate DMA-driven transmit and receive data paths.
