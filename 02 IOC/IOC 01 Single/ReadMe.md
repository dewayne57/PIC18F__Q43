# IOC 01 Single

## License
This material is provided free of charge on an "AS-IS" basis under the terms of the
Apache 2.0 License. 

```
Copyright (c) 2026, Dewayne Hafenstein.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

## Overview
This project demonstrates Interrupt-On-Change (IOC) on a PIC18F27Q43 using a flat
(non-hierarchical, single-vector) interrupt structure.

## Behavior

- PORTC is configured as digital inputs with weak pull-ups enabled.
- IOC is enabled on all PORTC pins on both rising and falling edges.
- On any IOC event, the ISR reads PORTC, inverts the value, and writes the result to PORTD.
- PORTD therefore shows the inverted real-time state of PORTC.

## Interrupt Model

- Config bit MVECEN is set to 0 for non-vectored interrupts.
- Runtime interrupt priority mode is disabled (IPEN = 0).
- A single ISR handles IOC events and clears IOC flags before returning.

## Source Files

- IOC/IOC 01 Single/main.c: Program entry point and main loop.
- IOC/IOC 01 Single/config.h: Device configuration bits and initialization prototype.
- IOC/IOC 01 Single/config.c: System initialization, port setup, IOC enable, and global interrupt enable.
- IOC/IOC 01 Single/ioc.c: Flat ISR implementation for IOC processing.

## Notes

- Initial output is set during initialization so PORTD immediately reflects inverted PORTC state.
- This is a minimal demonstration intended for straightforward IOC bring-up and testing.
