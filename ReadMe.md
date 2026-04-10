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

This example family is intended to use weak pull-ups on PORTC input pins and
drive LEDs on PORTD outputs corresponding to switch positions.

Expected behavior:

- A switch ON state shorts the associated PORTC pin to ground.
- The corresponding PORTD LED turns on.
- Firmware response is interrupt-driven using IOC events.

Projects:

- IOC 01 Single
- IOC 02 Vectored

Demonstrated concepts:

- Interrupt-On-Change configuration
- Single-level interrupt handling
- Vectored interrupt handling

### Signal Measurement Timer (SMT)

This example family explores multiple SMT operating patterns for measuring real
digital signals and extracting stable timing metrics.

Projects:

- SMT 01 Period Duty Capture
- SMT 02 Pulse Width Single Shot
- SMT 03 Windowed Frequency Meter
- SMT 04 Signal Quality Monitor
- SMT 05 Sleep Event Timestamping

Demonstrated concepts:

- Period and duty-cycle capture
- Pulse-width timing with timeout handling
- Windowed counting and jitter statistics
- Sleep-aware timestamping workflows

### Numerically Controlled Oscillator (NCO)

This example family demonstrates fixed and dynamic frequency generation using
the NCO peripheral, including measurement-assisted calibration.

Projects:

- NCO 01 Static Synthesizer
- NCO 02 Sweep Chirp Generator
- NCO 03 Closed Loop Calibration

Demonstrated concepts:

- Increment math and output frequency resolution
- Glitch-aware dynamic frequency updates
- Closed-loop correction using SMT feedback

### ADCC and DAC

This example family demonstrates analog acquisition and threshold control using
the ADCC and DAC peripherals.

Projects:

- ADCC 01 Window Comparator Hysteresis
- ADCC 02 Oversampling Filtering
- ADCC 03 DMA Sample Buffer
- DAC 01 Reference Ladder
- ADC DAC 02 Software Servo Threshold

Demonstrated concepts:

- Hardware threshold windows and hysteresis behavior
- Oversampling and filtering tradeoffs
- DMA-backed sample buffering
- DAC reference generation and ADC verification
- Closed-loop analog setpoint control

### Complementary Waveform Generator (CWG)

This example family focuses on safe power-stage style signal generation,
including dead-band control and hardware fault handling.

Projects:

- CWG 01 Complementary PWM Dead Band
- CWG 02 Auto Shutdown Restart
- CWG 03 Ramping Startup

Demonstrated concepts:

- Complementary outputs with dead-time insertion
- Hardware fault shutdown and restart behavior
- Controlled startup ramps for safer switching

### Cyclic Redundancy Check (CRC)

This family demonstrates hardware-assisted CRC generation and verification for
buffers and persistent data checks.

Projects:

- CRC 01 Stream Verify

Demonstrated concepts:

- Runtime CRC calculation on data streams
- Verification against expected signatures

### Nonvolatile Memory (NVM)

This family demonstrates safe flash write and verification workflows.

Projects:

- NVM 01 Flash Row Write Verify

Demonstrated concepts:

- Row erase and write sequencing
- Post-write integrity checks

### Windowed Watchdog Timer (WWDT)

This family demonstrates watchdog servicing within valid timing windows and
fault detection when service timing is violated.

Projects:

- WWDT 01 Window Service Monitor

Demonstrated concepts:

- Valid service windows
- Reset path verification

### Configurable Logic Cell (CLC)

This family demonstrates hardware logic mapping to reduce firmware load.

Projects:

- CLC 01 Hardware Logic Mapping

Demonstrated concepts:

- Combinational logic implementation
- Event routing through peripheral logic

### Timers

This family demonstrates timer usage across periodic interrupts, counting, and
one-shot timeout workflows.

Projects:

- TMR 01 Periodic Interrupt Tick
- TMR 02 External Counter Gate
- TMR 03 One Shot Timeout

Demonstrated concepts:

- Periodic scheduler tick generation
- Edge counting and gate timing
- One-shot timeout behavior

### Capture Compare PWM (CCP)

This family demonstrates capture and compare event handling.

Projects:

- CCP 01 Capture Compare Basics

Demonstrated concepts:

- Timestamp capture flow
- Compare event scheduling

### Pulse Width Modulation (PWM)

This family demonstrates PWM output generation and runtime duty updates.

Projects:

- PWM 01 Edge Aligned Duty Control

Demonstrated concepts:

- Duty cycle control
- Output timing validation

### Digital Signal Modulator (DSM)

This family demonstrates basic output modulation configurations.

Projects:

- DSM 01 Output Modulation Profile

Demonstrated concepts:

- Modulation source selection
- Output behavior characterization

### Universal Asynchronous Receiver Transmitter (UART)

This family demonstrates UART console and interrupt-driven receive paths.

Projects:

- UART 01 Interrupt Echo Console
- UART 02 DMA TX Stream
- UART 03 DMA RX Ring Buffer

Demonstrated concepts:

- Interrupt-driven RX handling
- Console command and echo flow
- DMA-assisted serial data movement

### Inter-Integrated Circuit (I2C)

This family demonstrates controller mode transactions and bus status handling.

Projects:

- I2C 01 Host Transaction Basics
- I2C 02 Bit Bang Master
- I2C 03 Module Slave Responder
- I2C 04 Module Master DMA

Demonstrated concepts:

- Start/address/data/stop flow
- ACK and error handling
- Bit-bang versus module-based operation
- DMA-assisted module master payload transfers

### Serial Peripheral Interface (SPI)

This family demonstrates SPI host transfers and loopback validation.

Projects:

- SPI 01 Host Loopback Transfer
- SPI 02 DMA Burst Transfer

Demonstrated concepts:

- Full-duplex transfer timing
- Data integrity checks
- DMA-backed burst transactions

### Fixed Voltage Reference (FVR)

This family demonstrates internal reference bring-up for analog subsystems.

Projects:

- FVR 01 Reference Bring Up

Demonstrated concepts:

- Reference level enable and settling
- Analog path integration checks

### Temperature Indicator

This family demonstrates internal temperature indicator read and trend use.

Projects:

- Temp 01 Internal Sensor Trend

Demonstrated concepts:

- Internal temperature acquisition
- Trend reporting and thresholds

### Comparator (CMP)

This family demonstrates comparator threshold events and interrupt response.

Projects:

- CMP 01 Threshold Interrupt Event

Demonstrated concepts:

- Threshold crossing detection
- Interrupt-driven event handling

### Zero Crossing Detector (ZCD)

This family demonstrates zero crossing event detection and timestamping.

Projects:

- ZCD 01 Zero Crossing Timestamp

Demonstrated concepts:

- Zero crossing event detection
- Timing correlation with other peripherals

## Getting Started

For any example project:

1. Open the desired MPLAB X project folder and review its ReadMe.
2. Confirm the selected device matches your hardware.
3. Build using XC8.
4. Program the target board.
5. Verify behavior against the project test procedure.

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