# ADCC and DAC Example Family

This family demonstrates analog acquisition and threshold control using ADCC and DAC.

## Implementation Status Notice
This peripheral family is not yet fully implemented. Current content is planning and
scaffold-focused; register-complete, validated demonstrations are still in progress.

## What It Is
ADCC is an analog-to-digital converter with computation features, and DAC is an
internal digital-to-analog reference/output source.

## What It Does
- ADCC samples analog inputs and can apply threshold and filtering logic.
- DAC generates programmable analog reference levels.
- Together they support closed-loop analog decision and control paths.

## Common Uses
- Sensor threshold alarms with hysteresis.
- Analog setpoint generation for comparator or control loops.
- Noise-tolerant telemetry and calibration workflows.
- Buffered acquisition pipelines using DMA.

## Projects
- ADCC 01 Window Comparator Hysteresis
- ADCC 02 Oversampling Filtering
- ADCC 03 DMA Sample Buffer
- DAC 01 Reference Ladder
- ADC DAC 02 Software Servo Threshold

## Shared Goals
- Demonstrate hardware thresholding and filtering.
- Show reference generation and analog verification loops.
- Provide templates for noise-robust sensing firmware.
- Demonstrate DMA-based sample buffering for lower CPU load.
