# SPI Example Family

This family demonstrates SPI host transfers with loopback verification.

## What It Is
Serial Peripheral Interface (SPI) is a synchronous full-duplex serial bus using
clocked data lines and chip-select signaling.

## What It Does
- Transfers bytes with deterministic clock timing.
- Supports high-throughput short-range peripheral links.
- Enables simultaneous transmit and receive.

## Common Uses
- ADC, DAC, display, and memory interfaces.
- Fast sensor acquisition links.
- Controller-to-co-processor data exchange.
- High-throughput burst transfers using DMA.

## Projects
- SPI 01 Host
- SPI 02 Host DMA
- SPI 03 Slave
- SPI 04 Slave DMA

## Shared Goals
- Demonstrate reliable peripheral initialization.
- Show observable behavior with measurable outputs.
- Provide reusable firmware patterns for future projects.
- Demonstrate DMA-backed full-duplex communication handling.
