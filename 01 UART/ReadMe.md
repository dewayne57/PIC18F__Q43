# UART Example Family

This family demonstrates UART communication patterns on the PIC18F47Q43, progressing
from a simple interrupt-driven echo console through DMA-offloaded transmit and two
hardware flow-control approaches.

## Configuration Settings

All projects use **19,200 baud, No parity, 8 data bits, 1 stop bit (19200 N 8 1)**.
Connect a terminal emulator (e.g. PuTTY, Tera Term, CoolTerm, RealTerm) using the 
same settings. 

The baud rate, parity, data width, and stop-bit count are all defined as named constants
in each project's `config.h` and can be changed without touching the initialization code.

## What It Is

Universal Asynchronous Receiver Transmitter (UART) is a serial communication peripheral
for byte-oriented, asynchronous, full-duplex data exchange.  Each data frame consists of
a start bit, a configurable number of data bits (7, 8, or 9), an optional parity bit,
and one or more stop bits.  The PIC18F47Q43 UART module includes a built-in baud rate
generator, a two-deep hardware FIFO, and hardware support for RTS/CTS and XON/XOFF flow
control.

The Q43 family offers several built-in UARTs.  Usually, only one (or a small number)
are full-function UARTs that support all capabilities.  The reduced-function UARTs
typically don't allow configuration of parity, stop bits, or other advanced settings.
Check the datasheet for the specific device you are using to confirm what is supported.

## Common Uses

- Debug consoles and interactive command shells.
- Device configuration and telemetry links.
- Bootloader and service-port communication.
- High-rate data streaming with DMA offload.

---

## Projects

### UART 01 — Interrupt Echo Console

**Folder:** `UART 01 Echo Console/`

Demonstrates the simplest useful UART pattern: receive a character, echo it back.
UART1 TX and RX are interrupt-driven using a pair of 128-byte software ring buffers.
`printf` output is routed through a `putch()` hook that queues characters into the TX
ring buffer and uses the TX-empty interrupt to drain it in the background.

The main loop polls `UART1_RxAvailable()`, reads each available character with
`UART1_ReadChar()`, and echoes it back via `printf`.  A one-second periodic counter
message is also printed to show that the main loop keeps running independently of the
UART activity.

**Key concepts:** interrupt-driven I/O, ring buffer (circular queue), non-blocking I/O,
`putch()` retarget for `printf`, PPS pin assignment.

**Hardware:** RB0 = TX1, RB1 = RX1 (2-wire, no flow control).

---

### UART 02 — DMA TX Stream

**Folder:** `UART 02 DMA TX/`

Demonstrates offloading UART transmit to the DMA engine so the CPU is free while bytes
are being shifted out.  A ping-pong double-buffer scheme keeps data flowing continuously:
the application fills one buffer while DMA drains the other to U1TXB.  When DMA finishes
a buffer the roles swap automatically.

`printf` output is routed through a `putch()` hook that appends bytes to the current fill
buffer.  When a line terminator (`\r\n` or `\n\r`) is detected, or when the fill buffer is
full, the buffer is handed to DMA for transmission.  A stall-detection watchdog and a
blocking fallback path (`UART_TX_SendBlocking`) guard against edge-case DMA errors.

**Key concepts:** DMA channel configuration, ping-pong buffering, hardware trigger
(UART1 TX FIFO space), interrupt priority levels (IPEN), DMA completion and error ISRs.

**Hardware:** RB0 = TX1, RB1 = RX1 (transmit-only demo; RX not included).

---

### UART 03 — Hardware RTS/CTS Flow Control

**Folder:** `UART 03 Hardware Flow Control/`

Extends the interrupt-driven echo (UART 01) with **hardware flow control**.
The UART module drives the RTS output automatically when the receive FIFO has space,
and monitors the CTS input to pause transmission when the remote device is busy.
No application code is needed to manage the handshake lines — the hardware handles it.

The main loop is identical to UART 01 (echo received characters and print a periodic
counter), but the UART is now a four-wire interface.

**Key concepts:** RTS/CTS hardware handshake (`U1CON2.FLO = 2`), PPS output assignment
for RTS (`RB2PPS`), PPS input assignment for CTS (`U1CTSPPS`).  Note, when flow 
control is needed, RTS/CTS (or Hardware flow control) is generally preferred. 

**Hardware:** RB0 = TX1, RB1 = RX1, RB2 = RTS1 (output), RB3 = CTS1 (input).

---

### UART 04 — Software XON/XOFF Flow Control

**Folder:** `UART 04 Software Flow Control/`

Extends the interrupt-driven echo (UART 01) with **software flow control**.
Instead of dedicated handshake wires, the UART hardware automatically inserts XOFF
(0x13) into the TX stream when its receive buffer is nearly full, and XON (0x11) when
space becomes available again.  The remote device must honour these control characters
to prevent data loss.  XON/XOFF characters are consumed by the hardware and do not
appear in the application receive buffer.

Terminal configuration note: set the host terminal's serial flow control mode to
**XON/XOFF (software)** for this example. If the terminal is set to no flow control
or to RTS/CTS, incoming characters may appear to stop or never arrive.

The main loop is identical to UART 01 (echo received characters and print a periodic
counter), using only the standard two-wire TX/RX connection.

**Key concepts:** XON/XOFF software flow control (`U1CON2.FLO = 1`), 2-wire interface
with in-band flow control, hardware automatic insertion of control characters.  Note, 
the use of software flow control requires using both RxD and TxD signals.  

**Hardware:** RB0 = TX1, RB1 = RX1 (2-wire; flow control is in-band).

---

## Shared Goals

- Demonstrate reliable UART peripheral initialization on the PIC18F47Q43.
- Show observable behavior verifiable with a standard terminal emulator.
- Provide reusable firmware patterns (ring buffers, DMA TX, flow control) for real projects.
- Progress from simple to advanced concepts across the four projects.
