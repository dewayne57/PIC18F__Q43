# PIC18F Q43 Family Examples

This repository is intended to hold examples and schematic/code fragments for the
Microchip PIC18F Q43 family. The goal is to demonstrate peripheral usage,
interrupt behavior, and practical embedded patterns that can be reused in other
projects.

## Supported Devices

Planned target devices include:

- PIC18F25Q43
- PIC18F26Q43
- PIC18F27Q43
- PIC18F45Q43
- PIC18F46Q43
- PIC18F47Q43
- PIC18F55Q43
- PIC18F56Q43
- PIC18F57Q43

Projects were originally developed and tested on PIC18F27Q43 and PIC18F47Q43.

## Toolchain

The development environment used for these examples:

- MPLAB X IDE 6.30
- MPLAB XC8 3.10
- Visual Studio Code 1.113
- KiCad 9.0.7

## Included Example Families

### Interrupt-On-Change (IOC)

This example family is intended to use weak pull-ups on PORTB input pins and
drive LEDs on PORTC outputs corresponding to DIP switch positions.

Expected behavior:

- A switch ON state shorts the associated PORTB pin to ground.
- The corresponding PORTC LED turns on.
- Firmware response is interrupt-driven using IOC events.

Projects:

- IOC Single
- IOC Vectored

Demonstrated concepts:

- Interrupt-On-Change configuration
- Single-level interrupt handling
- Vectored interrupt handling

## Getting Started

Once project source files are added:

1. Open the desired MPLAB X project (for example, IOC Single or IOC Vectored).
2. Confirm the selected device matches your hardware.
3. Build using XC8.
4. Program the target board.
5. Verify switch-to-LED behavior on PORTB/PORTC as documented above.

## Hardware Notes

- Use proper pull-up/pull-down assumptions for switch wiring.
- Ensure LED current-limiting resistors are present.
- Confirm IOC-enabled pins are configured as digital inputs.

## Contributing

Contributions are welcome. Suggested additions:

- Complete MPLAB X project files per example
- Schematic PDFs or KiCad source files
- Pin maps and board connection diagrams
- Test notes for each validated device

## License

This project is licensed under the Apache License 2.0. See LICENSE for details.