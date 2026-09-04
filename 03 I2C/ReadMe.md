# I2C Example Family

This family demonstrates I2C implementations using software bit-bang host and
internal hardware I2C host.  

## What It Is
Inter-Integrated Circuit (I2C) is a two-wire, address-based serial bus for
multi-device communication.

## What It Does
- Performs start/address/data/stop bus transactions.
- Handles acknowledge, arbitration, and bus-status conditions.
- Connects one controller to multiple addressed peripherals.

## Implementation Modes
- Bit-bang host: GPIO-driven I2C timing in software for protocol learning,
	timing experiments, or devices without a free hardware I2C peripheral.
- Internal I2C module host: Uses I2C hardware for robust bus timing,
	ACK/NACK handling, and lower CPU overhead.


## Common Uses
- Sensor and EEPROM interfaces.
- Board-level control IC communication.
- Low-pin-count peripheral expansion.

## Projects
- I2C 01 Bit Bang Host
- I2C 02 Module Host


## Shared Goals
- Demonstrate reliable peripheral initialization.
- Show observable behavior with measurable outputs.
- Provide reusable firmware patterns for future projects.
- Compare software and hardware I2C tradeoffs in timing and CPU load.
