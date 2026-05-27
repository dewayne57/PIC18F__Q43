# I2C Example Family

This family demonstrates I2C implementations using software bit-bang host,
internal hardware I2C host, and hardware I2C client operation.  It also demonstrates
the use of DMA for both host and client operation.

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
- Internal I2C module host: Uses MSSP/I2C hardware for robust bus timing,
	ACK/NACK handling, and lower CPU overhead.
- Internal I2C module client: Responds to bus requests from an external host
	and supports register-style or command-style peripheral emulation.
- Internal I2C module host with DMA: Uses hardware I2C plus DMA channel
	offload for multi-byte payload moves and reduced CPU intervention.
- Internal I2C module client with DMA: Uses hardware I2C plus DMA channel 
    offload for multi-byte payload moves and reduced CPU intervention. 

## Common Uses
- Sensor and EEPROM interfaces.
- Board-level control IC communication.
- Low-pin-count peripheral expansion.

## Projects
- I2C 01 Bit Bang Host
- I2C 02 Module Host
- I2C 03 Module Client 
- I2C 04 Module Host DMA
- I2C 05 Module Client DMA 

## Shared Goals
- Demonstrate reliable peripheral initialization.
- Show observable behavior with measurable outputs.
- Provide reusable firmware patterns for future projects.
- Compare software and hardware I2C tradeoffs in timing and CPU load.
- Provide reference patterns for both bus-host and bus-client roles.
- Demonstrate DMA-assisted payload transfer.
