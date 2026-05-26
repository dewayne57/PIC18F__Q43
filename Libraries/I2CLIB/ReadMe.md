# I2C Library

Interrupt-driven I2C library for the PIC18F Q43 family using the I2C1 peripheral.
The library supports master-mode transactions with 7-bit and 10-bit addressing
through one common API and an application-owned i2c_handle_t structure.

## Intended Use

The I2C projects under 03 I2C/ are kept as simple, self-contained demonstration
projects and are not intended to be rewritten around this library. This library
is meant to be consumed by other peripheral example projects when they need
shared I2C transaction support.

## Files

| File | Purpose |
|------|---------|
| i2clib.h | Public API, enums, address types, and i2c_handle_t |
| i2clib.c | Implementation for I2C1 master operations and weak vectored ISRs |

## Design Summary

- The application owns all runtime state using i2c_handle_t.
- The library does not allocate memory.
- Caller-provided TX and RX buffers are tracked in the handle for ISR-driven
  transfers.
- The current implementation configures and uses I2C1 hardware pins on RC3/RC4.
- The implementation currently supports master transactions:
  - i2c_writeSlave()
  - i2c_readSlave()
  - i2c_writeReadSlave()
- The library provides weak vectored ISR handlers for general, error, TX,
  and RX I2C1 interrupts.

## Current Scope and Limitations

- Current implementation focus is I2C_MODE_MASTER_7BIT and
  I2C_MODE_MASTER_10BIT.
- Slave and multi-master configuration paths are present in enums and switch
  blocks but not fully implemented yet.
- i2c_handle_t includes a channel field, but the current implementation is tied
  to I2C1 hardware.
- Header declarations exist for i2c_getStatus(), i2c_writeMaster(), and
  i2c_readMaster(), but these are not implemented yet in i2clib.c.

## Adding the Library to a Project

1. Add ../../libraries/i2clib/i2clib.h to your MPLAB X project as an external
   include file, or define ../../libraries/i2clib as an include search path.
2. Add ../../libraries/i2clib/i2clib.o to your link input as an external
   object file, or add ../../libraries/i2clib as an object search path.
3. Include the header where your I2C handle is defined:

```c
#include "i2clib.h"
```

## Build and Target Note

- This library uses compile-time checks and register definitions from xc.h and
  the selected device pack.
- If i2clib.c is compiled as part of each project, it is rebuilt for that
  specific project target and options.
- If you link a prebuilt i2clib.o, it is valid only for the target/device
  settings used when that object was built.

## Example Handle Setup

```c
#include "i2clib.h"

static uint8_t i2c_tx_buf[32];
static uint8_t i2c_rx_buf[32];

static i2c_handle_t i2c1_handle = {
  .mode = I2C_MODE_MASTER_7BIT,
  .channel = 1,
  .speed_khz = 400,
  .initialized = false,
};
```

## Initialization

```c
i2c_status_t st = i2c_init(&i2c1_handle, 1, I2C_MODE_MASTER_7BIT, 400);
if (st == I2C_SUCCESS)
{
  INTCON0bits.GIE = 1;
}
```

## Transfer API Examples

Write bytes to a slave:

```c
const uint8_t tx_data[] = { 0x10, 0x55, 0xAA };
i2c_status_t st = i2c_writeSlave(&i2c1_handle, 0x50, tx_data, sizeof(tx_data));
```

Read bytes from a slave:

```c
uint8_t rx_data[8];
i2c_status_t st = i2c_readSlave(&i2c1_handle, 0x50, rx_data, sizeof(rx_data));
```

Write then read (common register-address transaction):

```c
uint8_t reg = 0x00;
uint8_t rx_data[4];
i2c_status_t st = i2c_writeReadSlave(&i2c1_handle, 0x50, &reg, 1, rx_data, sizeof(rx_data));
```

## Public API Summary

| Function | Purpose |
|----------|---------|
| i2c_init() | Initializes I2C1 hardware and validates/initializes the handle |
| i2c_writeSlave() | Starts a master write transaction to a slave address |
| i2c_readSlave() | Starts a master read transaction from a slave address |
| i2c_writeReadSlave() | Starts a combined write-then-read transaction |
| i2c_getStatus() | Declared in header; implementation pending |
| i2c_writeMaster() | Declared in header; implementation pending |
| i2c_readMaster() | Declared in header; implementation pending |

## Interrupt Model

The library defines weak, high-priority vectored ISRs for I2C1:

- i2c1_generalISR()
- i2c1_errorISR()
- i2c1_transmitISR()
- i2c1_receiveISR()

Default behavior is to use these handlers directly.
If application code overrides them, it must preserve equivalent I2C state
management behavior.

## Integration Checklist

1. Add i2clib.h and i2clib.o to your MPLAB X project.
2. Define an i2c_handle_t and any transaction buffers your project needs.
3. Call i2c_init() at startup with the desired mode and speed.
4. Enable global interrupts in the application.
5. Use i2c_writeSlave(), i2c_readSlave(), and i2c_writeReadSlave() for
   transactions.

## Notes

- The library configures RC3/RC4 for I2C1 PPS and open-drain operation.
- _XTAL_FREQ is used by delay macros. If not defined by the project, i2clib.c
  currently falls back to 64000000UL.
- Error ISR handling currently maps NACK and bus-collision conditions to
  I2C_ERROR_NACK_RECEIVED and I2C_ERROR_BUS_COLLISION.
- This library is a shared foundation and still evolving with additional slave,
  multi-master, and status/query features.
