# I2C 02 Bit Bang Master

## Overview
Implement I2C master transactions entirely in software using GPIO for SCL and SDA.

## Status
Planned. Source and schematic capture are next steps.

## Suggested Milestones
1. Define SDA and SCL pins with open-drain and pull-up behavior.
2. Implement start, stop, write-bit, read-bit, and ACK sampling primitives.
3. Add byte-level read and write helpers with clock-stretch timeout handling.
4. Validate bus timing and interoperability with a known I2C slave device.
