# DMA Library

Shared DMA helper functions for PIC18F Q43 example projects.

## Files

- `dmalib.h` - public API for DMA channel selection and source/destination address helpers
- `dmalib.c` - implementation of the helper functions

## Purpose

This library provides a small reusable wrapper around the PIC18 DMA registers so example
projects can share the same helper code without importing DMA support from another example.

## Public API

- `DMA_IsTransferInProgress()`
- `DMA_SelectChannel()`
- `DMA_SetSourceAddress()`
- `DMA_SetDestAddress()`
- `DMA_SetTransferCount()`

## Integration

Add both source files to the project that needs DMA support and add `Libraries/dmalib`
to the include search path.

## Notes

The helpers are intentionally thin wrappers over the DMA SFRs. They do not implement any
interrupt policy or transfer scheduling; the consuming project owns that logic.
