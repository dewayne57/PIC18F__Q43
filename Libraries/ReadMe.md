# Libraries

This folder contains reusable support libraries intended to be shared by the
peripheral example projects in this repository.

The libraries here are different from the demonstration projects under the
numbered peripheral folders such as `01 UART/`, `03 I2C/`, or `04 SPI/`.
Those folders are meant to stay simple and focused on teaching one peripheral
feature at a time. The `Libraries/` folder is for reusable building blocks
that other example projects can consume when they need common support services
such as debug output.

## Current Libraries

| Library | Purpose | Typical Use |
|---------|---------|-------------|
| `UARTLIB` | Shared interrupt-driven UART library supporting UART1-UART5 on PIC18F Q43/Q84 devices | Debug console output, status logging, printf retargeting, lightweight serial diagnostics |
| `DMALIB` | Shared DMA helper functions for PIC18F Q43 examples | DMA channel selection, source/destination address setup, transfer count setup, active-transfer polling |

## Library Usage Pattern

In general, libraries in this folder should follow these rules:

- They should be reusable across multiple example projects.
- They should avoid hiding board-specific pin setup that belongs in the consuming project.
- They should document required configuration, limitations, and a minimal integration example.
- They should not make the standalone demonstration projects harder to understand.

## Per-Library Documentation

Each library folder should contain its own `ReadMe.md` describing:

- what the library does
- what files it contains
- how to add it to a project
- how to configure and initialize it
- any device-specific limits or assumptions
- a short usage example

Currently available detailed documentation:

- `UARTLIB/ReadMe.md` - Multi-UART debug/output library documentation
- `DMALIB/ReadMe.md` - Shared DMA helper library documentation