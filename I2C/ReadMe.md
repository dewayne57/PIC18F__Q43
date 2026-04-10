# I2C Example Family

This family demonstrates I2C implementations using software bit-bang master,
internal hardware I2C master, and hardware I2C slave operation.

## What It Is
Inter-Integrated Circuit (I2C) is a two-wire, address-based serial bus for
multi-device communication.

## What It Does
- Performs start/address/data/stop bus transactions.
- Handles acknowledge, arbitration, and bus-status conditions.
- Connects one controller to multiple addressed peripherals.

## Implementation Modes
- Bit-bang master: GPIO-driven I2C timing in software for protocol learning,
	timing experiments, or devices without a free hardware I2C peripheral.
- Internal I2C module master: Uses MSSP/I2C hardware for robust bus timing,
	ACK/NACK handling, and lower CPU overhead.
- Internal I2C module slave: Responds to bus requests from an external master
	and supports register-style or command-style peripheral emulation.
- Internal I2C module master with DMA: Uses hardware I2C plus DMA channel
	offload for multi-byte payload moves and reduced CPU intervention.

## Common Uses
- Sensor and EEPROM interfaces.
- Board-level control IC communication.
- Low-pin-count peripheral expansion.

## Projects
- I2C 01 Host Transaction Basics
- I2C 02 Bit Bang Master
- I2C 03 Module Slave Responder
- I2C 04 Module Master DMA

## Shared Goals
- Demonstrate reliable peripheral initialization.
- Show observable behavior with measurable outputs.
- Provide reusable firmware patterns for future projects.
- Compare software and hardware I2C tradeoffs in timing and CPU load.
- Provide reference patterns for both bus-master and bus-slave roles.
- Demonstrate DMA-assisted payload transfer for hardware master mode.
