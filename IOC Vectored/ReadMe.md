# IOC Vectored

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
This project demonstrates Interrupt-On-Change (IOC) on a PIC18F27Q43 using the vectored,
priority interrupt system.

## Behavior

- PORTD is configured as digital inputs with weak pull-ups enabled.
- IOC is enabled on all PORTD pins on both rising and falling edges.
- On any IOC event, the ISR reads PORTD, inverts the value, and writes the result to PORTC.
- PORTC therefore shows the inverted real-time state of PORTD.

## Interrupt Model

- Config bit MVECEN is set to 1 for vectored interrupts.
- Runtime interrupt priority mode is enabled (IPEN = 1).
- An IOC-specific interrupt service routine handles all IOC events on a low-priority 
  basis.  If other interrupts are handled, they will have their own specific interrupt 
  service routines.

## Source Files

- IOC_Vectored/main.c: Program entry point and main loop.
- IOC_Vectored/config.h: Device configuration bits and initialization prototype.
- IOC_Vectored/config.c: System initialization, port setup, IOC enable, and global interrupt enable.
- IOC_Vectored/ioc.c: Flat ISR implementation for IOC processing.

## Notes

- Initial output is set during initialization so PORTC immediately reflects inverted PORTD state.
- This is a minimal demonstration intended for straightforward IOC bring-up and testing.
