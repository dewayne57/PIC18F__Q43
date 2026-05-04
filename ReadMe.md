# PIC18F Q43 Family Examples

This repository is intended to provide examples and schematic/code fragments for the
Microchip PIC18F Q43 family. The goal is to demonstrate peripheral usage,
interrupt behavior, and practical embedded patterns that can be reused in other
projects.

Therefore, the circuits tend to be as simple as possible, favoring older 
technology external devices to ensure a hobbyist can obtain the needed parts.
For example, when op-amps are used, I tend to favor older, simpler devices such 
as OP07, LM1448, TL062, TL072, LM334, etc.  The same is true for other devices 
as well. The goal is to demonstrate the use of the specific peripheral, not to 
dazzle the reader with sophistication and complexity.

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

Projects were originally developed and tested on PIC18F27Q43 and PIC18F47Q43.  In 
most cases, the 47Q43 (TQFP-44) device is shown in the schematics.  This was for 
convenience to me.  I have also tested all the circuits with the 27Q43 device, 
but in some cases required modifications to the schematic (mainly because of 
lack of ports or conflicts with the ICSP pins). I avoided using jumpers or other 
means of disconnection of the ICSP pins to allow the user to run the code under 
the debugger with full functionality. 

## Toolchain

The development environment used for these examples:

- MPLAB X IDE 6.30
- MPLAB XC8 3.10
- Visual Studio Code 1.113
- KiCad 10.0.1

## Included Example Families

These modules demonstrate how to use various peripherals of the Q43 family.  In order to 
demonstrate many of these features it is easiest to have the ability to send diagnostic 
messages to a serial UART connected to a terminal emulator.  This allows the code to 
write messages that help you understand how the code works. 

Therefore, the first module is dedicated to using the UART.  This module will be used 
as part of all other modules to allow logging.  

The various groups of projects that demonstrate a specific peripheral are numbered 
with a 2-digit prefix.  This numbering scheme is the order I felt made the most 
sense to allow a new user to the Q43 to become familiar with the family. A user 
already familiar with the Q43 family can safely use these demonstrations in any order.

### 01 - Universal Asynchronous Receiver Transmitter (UART)

This family demonstrates UART console and interrupt-driven receive paths.  There is a 
fair amount of information available that demonstrates using the UARTs in a blocking 
approach.  This is simple to implement but almost never used in actual applications.
The reason for this is that when the I/O operation is started, the code "blocks" 
execution of other functions by looping and/or waiting for the I/O operation to 
complete.  This essentially reduces the ability of the controller to perform 
processing to the speed of the UART, which is terribly slow compared to the actual 
capabilities of the controller.

Instead, non-blocking approaches are almost always used.  This means the I/O 
operation is started and the processing performed by the controller continues.  The 
I/O operation continues asynchronously to the controller processing.  We then need 
to know when the I/O operation is completed and start the next I/O operation, again 
to proceed asynchronously.  This relies on hardware capabilities built-in to the 
controller to generate interrupts when various conditions are reached. 

Therefore, I am not going to show blocking implementations but instead focus on 
interrupt-driven and DMA (asynchronous) approachs to read and write serial data.

Projects:

- UART 01 Interrupt Echo Console: This module demonstrates the use of the UART in an
  interrupt-driven read and write implementation, implementing putch() for use of C runtime
  functions, and the echo of received data.

- UART 02 DMA TX: This demonstration shows the use of DMA (Direct Memory Access) to 
  transfer data from an in-memory buffer using the hardware DMA facility to manage the 
  transfer of the data.  DMA offloads the software by setting up the hardware to do the 
  transfers automatically. 

- UART 03 DMA RX: This module keeps the DMA TX stream path and adds DMA RX ping-pong
  buffers, exposing an API to check when a receive buffer is available and retrieve it for
  processing. The UART 01 module uses an interrupt-managed ring buffer; this example
  demonstrates an alternative receive path where DMA performs the byte movement into memory.
  This project also introduces the use of hardware flow control (RTS/CTS) to prevent
  buffer overruns. 

Demonstrated concepts:

- Interrupt-driven TX/RX handling
- Console command and echo flow
- DMA-assisted serial data movement
- Hardware flow control

### 02 - Interrupt-On-Change (IOC)

This example family is intended to use weak pull-ups on PORTC input pins and drive LEDs on 
PORTD outputs corresponding to switch positions.

Expected behavior:

- A switch ON state shorts the associated PORTC pin to ground.
- The corresponding PORTD LED turns on.
- Firmware response is interrupt-driven using IOC events.

Projects:

- IOC 01 Single: Demonstrates handling interrupt on change using the legacy "flat" interrupt 
  structure. 
- IOC 02 Vectored: Demonstrates handling interrupt on change using the priority vectored 
  interrupt system. 

Demonstrated concepts:

- Interrupt-On-Change configuration
- Single-level interrupt handling
- Prioritized vectored interrupt handling

### 03 - Inter-Integrated Circuit (I2C)

This family demonstrates I2C communication using both bit-bang and module-based approaches,
covering host and peripheral (slave) roles as well as DMA-assisted transfers.

Projects:

- I2C 01 Bit Bang Master: Implements I2C host transactions in software without the hardware
  module, demonstrating start/stop conditions, byte transfer, and ACK handling.
- I2C 02 Module Master: Demonstrates host mode transactions using the hardware I2C module,
  including address, data, and error handling.
- I2C 03 Module Slave: Demonstrates peripheral (responder) mode using the hardware I2C
  module, handling address match and data exchange.
- I2C 04 Module Master DMA: Extends module master operation with DMA-assisted payload
  transfers to reduce firmware overhead on bulk transactions.
- I2C 05 Module Slave DMA: Extends module slave operation with DMA-assisted data movement
  for higher-throughput peripheral responses.

Demonstrated concepts:

- Start/address/data/stop flow
- ACK and error handling
- Bit-bang versus module-based operation
- Host and peripheral (slave) roles
- DMA-assisted module transfers

### 04 - Serial Peripheral Interface (SPI)

This family demonstrates SPI host transfers and loopback validation.

Projects:

- SPI 01 Host Loopback Transfer
- SPI 02 DMA Burst Transfer

Demonstrated concepts:

- Full-duplex transfer timing
- Data integrity checks
- DMA-backed burst transactions

### 05 - Cyclic Redundancy Check (CRC)

This family demonstrates hardware-assisted CRC generation and verification for
buffers and persistent data checks.

Projects:

- CRC 01 Stream Verify

Demonstrated concepts:

- Runtime CRC calculation on data streams
- Verification against expected signatures

### 06 - Fixed Voltage Reference (FVR)

This family demonstrates internal reference bring-up for analog subsystems.

Projects:

- FVR 01 Reference Bring Up

Demonstrated concepts:

- Reference level enable and settling
- Analog path integration checks

### 07 - ADCC and DAC

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

### 08 - Temperature Indicator

This family demonstrates internal temperature indicator read and trend use.

Projects:

- Temp 01 Internal Sensor Trend

Demonstrated concepts:

- Internal temperature acquisition
- Trend reporting and thresholds

### 09 - Timers

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

### 10 - Comparator (CMP)

This family demonstrates comparator threshold events and interrupt response.

Projects:

- CMP 01 Threshold Interrupt Event

Demonstrated concepts:

- Threshold crossing detection
- Interrupt-driven event handling

### 11 - Configurable Logic Cell (CLC)

This family demonstrates hardware logic mapping to reduce firmware load.

Projects:

- CLC 01 Hardware Logic Mapping

Demonstrated concepts:

- Combinational logic implementation
- Event routing through peripheral logic

### 12 - Numerically Controlled Oscillator (NCO)

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

### 13 - Pulse Width Modulation (PWM)

This family demonstrates PWM output generation and runtime duty updates.

Projects:

- PWM 01 Edge Aligned Duty Control

Demonstrated concepts:

- Duty cycle control
- Output timing validation

### 14 - Complementary Waveform Generator (CWG)

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

### 15 - Digital Signal Modulator (DSM)

This family demonstrates basic output modulation configurations.

Projects:

- DSM 01 Output Modulation Profile

Demonstrated concepts:

- Modulation source selection
- Output behavior characterization

### 16 - Capture Compare PWM (CCP)

This family demonstrates capture and compare event handling.

Projects:

- CCP 01 Capture Compare Basics

Demonstrated concepts:

- Timestamp capture flow
- Compare event scheduling

### 17 - Signal Measurement Timer (SMT)

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

### 18 - Zero Crossing Detector (ZCD)

This family demonstrates zero crossing event detection and timestamping.

Projects:

- ZCD 01 Zero Crossing Timestamp

Demonstrated concepts:

- Zero crossing event detection
- Timing correlation with other peripherals

### 19 - Nonvolatile Memory (NVM)

This family demonstrates safe flash write and verification workflows.

Projects:

- NVM 01 Flash Row Write Verify

Demonstrated concepts:

- Row erase and write sequencing
- Post-write integrity checks

### 20 - Windowed Watchdog Timer (WWDT)

This family demonstrates watchdog servicing within valid timing windows and
fault detection when service timing is violated.

Projects:

- WWDT 01 Window Service Monitor

Demonstrated concepts:

- Valid service windows
- Reset path verification

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