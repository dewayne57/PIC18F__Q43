# IOC Single

This project demonstrates Interrupt-On-Change (IOC) on a PIC18F27Q43 using a flat
(non-hierarchical, single-vector) interrupt structure.

## Behavior

- PORTD is configured as digital inputs with weak pull-ups enabled.
- IOC is enabled on all PORTD pins on both rising and falling edges.
- On any IOC event, the ISR reads PORTD, inverts the value, and writes the result to PORTC.
- PORTC therefore shows the inverted real-time state of PORTD.

## Interrupt Model

- Config bit MVECEN is set to 0 for non-vectored interrupts.
- Runtime interrupt priority mode is disabled (IPEN = 0).
- A single ISR handles IOC events and clears IOC flags before returning.

## Source Files

- IOC_Single/main.c: Program entry point and main loop.
- IOC_Single/config.h: Device configuration bits and initialization prototype.
- IOC_Single/config.c: System initialization, port setup, IOC enable, and global interrupt enable.
- IOC_Single/ioc.c: Flat ISR implementation for IOC processing.

## Notes

- Initial output is set during initialization so PORTC immediately reflects inverted PORTD state.
- This is a minimal demonstration intended for straightforward IOC bring-up and testing.
