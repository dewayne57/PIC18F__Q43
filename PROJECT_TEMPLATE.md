# {{PROJECT_NAME}}

## License
This material is provided free of charge on an "AS-IS" basis under the terms of the
Apache 2.0 License.

## Overview
{{OVERVIEW}}

## Peripheral Focus
- {{PERIPHERAL_1}}
- {{PERIPHERAL_2}}

## Hardware Connections
- Device: {{DEVICE}}
- Signal input pins: {{INPUT_PINS}}
- Signal output pins: {{OUTPUT_PINS}}
- Optional loopback jumper(s): {{LOOPBACKS}}

## Behavior
- {{BEHAVIOR_1}}
- {{BEHAVIOR_2}}
- {{BEHAVIOR_3}}

## Interrupt and Event Model
- {{INT_MODEL_1}}
- {{INT_MODEL_2}}

## Source Files
- main.c: program entry point and superloop behavior.
- config.h: config bits and initialization prototypes.
- config.c: oscillator, ports, peripheral setup.
- {{FEATURE_FILE}}: feature-specific ISR and helper routines.

## Test Procedure
1. Build with XC8 and program the target.
2. Apply the documented stimulus.
3. Observe measured values on UART and indicator pins.
4. Verify expected timing and edge-case behavior.

## Expected Results
- {{EXPECTED_1}}
- {{EXPECTED_2}}

## Extensions
- {{EXTENSION_1}}
- {{EXTENSION_2}}
