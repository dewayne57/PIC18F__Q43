# NCO Example Family

This family demonstrates Numerically Controlled Oscillator generation and calibration flows.

## Implementation Status Notice
This peripheral family is not yet fully implemented. Current content is planning and
scaffold-focused; register-complete, validated demonstrations are still in progress.

## What It Is
Numerically Controlled Oscillator (NCO) is a hardware phase-accumulator block
that generates deterministic output frequencies from an increment value.

## What It Does
- Produces programmable output frequencies with fine resolution.
- Supports stable frequency synthesis from a reference clock.
- Allows runtime frequency updates for sweeps and modulation profiles.

## Common Uses
- Test signal generation and frequency references.
- Chirp and sweep sources for characterization.
- Software-calibrated clock or tone generation.

## Projects
- NCO 01 Static Synthesizer
- NCO 02 Sweep Chirp Generator
- NCO 03 Closed Loop Calibration

## Shared Goals
- Demonstrate increment math and frequency resolution.
- Provide generation profiles from fixed to dynamic waveforms.
- Show practical calibration against SMT measurements.
