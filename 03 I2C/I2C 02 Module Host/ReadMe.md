# I2C 02 Module Host

## License
This material is provided free of charge on an AS-IS basis under the Apache 2.0 License.

## Overview
This project implements an I2C host using the PIC18F47Q43 hardware I2C1 module on GPIO pins RC3 (SCL) and RC4 (SDA). It communicates with an MCP23017 I/O expander, mirrors Port A switch inputs to Port B LEDs, and reports configuration changes over UART. The application logic is identical to I2C 01 Bit Bang Host; only the I2C driver layer is replaced with the hardware peripheral.

## Peripheral Focus
- Hardware I2C1 module host (100 kHz, vectored interrupts)
- MCP23017 I/O expander
- UART1 debug console
- External interrupt input (INT1 on RB2)

## Hardware Connections
- Device: PIC18F47Q43
- I2C SCL: RC3 (open-drain with pull-up, routed via PPS to I2C1SCL)
- I2C SDA: RC4 (open-drain with pull-up, routed via PPS to I2C1SDA)
- UART TX/RX: RB0/RB1 at 19200,8,N,1
- MCP23017 INT1 output: RB2 (configured active high)
- MCP23017 Port A: DIP switch inputs
- MCP23017 Port B: LED outputs

## Behavior
- Initializes the hardware I2C1 module and configures MCP23017 in banked mode.
- Sets MCP23017 interrupt polarity active high (IOCON = 0x82).
- Performs an initial read of GPIOA and writes inverted value to OLATB.
- On each INT1 event, reads GPIOA and writes inverted value to OLATB.
- Reports input changes on UART as `Config is now %02X`.

## Interrupt and Event Model
- UART1 RX and TX are handled by vectored ISRs (`IRQ_U1RX`, `IRQ_U1TX`) that call UART library handlers.
- MCP23017 INT1 asserts RB2, which triggers `IRQ_INT1`.
- The INT1 ISR performs I2C read/write and queues a pending UART report value.
- `APP_Service()` runs in the superloop and prints queued configuration updates.

## Source Files
- `main.c`: system startup, UART open/select, superloop, and echo test path.
- `config.h`: config bits and system constants.
- `config.c`: oscillator, port setup, PPS routing (I2C1SCL/SDA + INT1), and interrupt configuration.
- `i2c.c`/`i2c.h`: hardware I2C1 module driver (interrupt-driven, 7-bit host mode; the API takes 8-bit write-form addresses).
- `app.c`/`app.h`: MCP23017 initialization, INT1 ISR behavior, and UART status reporting.
- `mcp23x17.h`: MCP23017 register definitions.
- `../../Libraries/UARTLIB`: shared UART driver used by this project.

## I2C1 Module Configuration
- Clock source: MFINTOSC (`I2C1CLK = 0b00011`)
- Bus rate: fixed 100 kHz standard mode for this example
- Mode: 7-bit host (`I2C1CON0bits.MODE = 0b100`)
- Addressing: callers pass the 8-bit write-form address; the driver toggles the low-order bit for read/write transfers.
- Address buffer: hardware-managed (`I2C1CON2bits.ABD = 0`)
- SDA hold time: 300 ns (`I2C1CON2bits.SDAHT = 0b01`)

## Test Procedure
1. Program the device with this project.
2. Connect serial terminal at 19200,8,N,1.
3. Verify startup messages:

```text
I2C 02 Module Host
Initializing MCP23017 I/O expander
I2C initialized successfully
Config is now XX
```

4. Toggle DIP switches on MCP23017 Port A.
5. Confirm LED pattern on Port B tracks the switch state (after inversion for active-low drive).
6. Confirm UART prints a new `Config is now %02X` line whenever the value changes.

## Expected Results
- I2C transactions ACK and complete without bus lockup.
- RC3/RC4 idle high (released) and toggle during transfers.
- Port B updates immediately at startup and on each interrupt-driven change.
- UART continuously reports changed Port A values.

## Notes
- This project uses the shared UART library at `../../Libraries/UARTLIB`.
- Compared to I2C 01 Bit Bang Host, this project offloads all I2C timing and signalling to the I2C1 hardware peripheral, freeing the CPU from bit-banging delays.
