# CWG Example Family

This family demonstrates complementary waveform generation and fault-safe behavior using CWG.

## What It Is
Complementary Waveform Generator (CWG) is a hardware output stage helper for
producing complementary drive signals with dead-time and safety features.

## What It Does
- Generates paired output waveforms for half-bridge style drive.
- Inserts dead-band to prevent shoot-through.
- Supports fast hardware shutdown on fault events.

## Common Uses
- Motor and power converter gate-drive interfaces.
- Safe PWM distribution to external driver stages.
- Fault-tolerant switching control designs.

## Projects
- CWG 01 Complementary PWM Dead Band
- CWG 02 Auto Shutdown Restart
- CWG 03 Ramping Startup

## Shared Goals
- Demonstrate safe half-bridge style output control.
- Show hardware fault response and deterministic shutdown.
- Provide reusable startup and restart policy patterns.
