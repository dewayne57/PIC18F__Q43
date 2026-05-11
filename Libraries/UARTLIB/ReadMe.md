# UART Library

Interrupt-driven UART library for the PIC18F Q43/Q84 family. The library 
supports UART1 through UART5 with one common API. Each UART instance is defined
by an application-owned `uart_handle_t` structure that contains both the UART
configuration and the ring-buffer state for that specific peripheral.

## Intended Use

The UART projects under `01 UART/` are kept as simple, self-contained
demonstration projects and are not intended to be rewritten around this library.
This library is meant to be consumed by other peripheral example projects when
they need UART-based debug output, console text, or lightweight serial logging.

Note that the Q84 family has built-in support for several serial protocols, 
including DMX, LIN, DALI, and Manchester encoding.  This library does not 
support these features as it is intended to be used only within these 
demonstration projects. 

## Files

| File | Purpose |
|------|---------|
| `uartlib.h` | Public API, enums, and the `uart_handle_t` instance structure |
| `uartlib.c` | Implementation, generic interrupt handlers, and `putch()` support |

## Design Summary

- The application owns UART state. The application is responsible to allocate 
all needed buffers.  The library does not allocate them.
- One `uart_handle_t` is created for each UART peripheral in use.
- The uart handle data area MUST NOT be altered by the application after it has been
initialized/opened. If the UART must be altered, call `UART_Close()`, update
the handle fields, then call `UART_Open()` to reinitialize the UART.
- The handle selects UART1, UART2, UART3, UART4, or UART5 through `port`.
- TX and RX buffer storage is supplied by the application through pointers in the handle.
- The same source files can manage multiple UART peripherals in one project.

## Important Limitation

On Q43 devices, only UART1 is full-featured. This library therefore accepts
`UART_FLOW_XON_XOFF` and `UART_FLOW_RTS_CTS` only when `port = UART_PORT_1`.
UART2 through UART5 are supported for the common TX/RX ring-buffer use case with
`UART_FLOW_NONE`.

## Adding the Library to a Project

1. Add `../../libraries/uartlib/uartlib.h` to your MPLAB X project as an external
   include file, or define `../../libraries/uartlib` as an include search path.
2. Add `../../libraries/uartlib/uartlib.o` to your link input as an external 
   object file, or add `../../libraries/uartlib` as an object search path. 
3. Include the header where the UART handles are defined:

```c
#include "uartlib.h"
```

4. Create one TX buffer, one RX buffer, and one `uart_handle_t` for each UART you need.
   The buffer sizes must be a power of 2 and at least 2 bytes in length, such as 
   2, 4, 8, 16, 32, 64, 128, 256, etc.

## Example Handle Setup

```c
static char console_tx_buffer[128];
static char console_rx_buffer[128];

static uart_handle_t console_uart = {
  .port = UART_PORT_1,
  .high_speed_baud = false,
  .baud_rate = 19200U,
  .fosc = _XTAL_FREQ,
  .data_bits = 8U,
  .parity = UART_PARITY_NONE,
  .stop_bits = UART_STOP_BITS_1,
  .flow_control = UART_FLOW_NONE,
  .tx_buffer = console_tx_buffer,
  .tx_buffer_size = sizeof(console_tx_buffer),
  .rx_buffer = console_rx_buffer,
  .rx_buffer_size = sizeof(console_rx_buffer),
  .tx_head = 0U,
  .tx_tail = 0U,
  .rx_head = 0U,
  .rx_tail = 0U,
  .initialized = false
};
```

## Buffer Rules

- `tx_buffer_size` and `rx_buffer_size` must be powers of 2.
- One slot in each ring buffer is always reserved to distinguish full from empty.
- The buffers are owned by the application, not by the library.

## Pin Setup

The library is pin-agnostic. Configure TRIS and PPS for the selected UART in the
application before calling `UART_Open()`.

Example for UART1 on RB0/RB1:

```c
TRISBbits.TRISB0 = 0;
TRISBbits.TRISB1 = 1;

PPS_Unlock();
RB0PPS = 0x20;
U1RXPPS = 0x09;
PPS_Lock();
```

## Initialisation

```c
if (UART_Open(&console_uart))
{
  INTCON0bits.GIE = 1;
}
```

## Open/Close Lifecycle

- `uart_handle_t.initialized` is the open/closed state bit.
  `true` means open and active, `false` means closed.
- `UART_Open()` validates the handle, initializes hardware, resets ring-buffer indices,
  and sets `initialized = true`.
- `UART_Open()` called on an already-open handle does nothing and returns success.
- `UART_Close()` disables UART interrupts, turns the UART module off,
  and sets `initialized = false`.
- `UART_Close()` called on an already-closed handle does nothing.
- To reconfigure an existing UART instance:
  1. Call `UART_Close(&handle);`
  2. Update handle fields (`baud_rate`, `fosc`, framing, flow control, etc.)
  3. Call `UART_Open(&handle);`

### Baud Rate Formula

| Speed mode | Formula |
|-----------|---------|
| Standard (`high_speed_baud = false`) | `BRG = (Fosc / (16 * baud)) - 1` |
| High speed (`high_speed_baud = true`) | `BRG = (Fosc / (4 * baud)) - 1` |

`high_speed_baud` maps directly to the UART BRGS bit.

For baud rates of 115200 and above, high-speed mode is often required for reliable operation
depending on oscillator tolerance and link conditions. If standard mode is unstable at high
baud rates, set `high_speed_baud = true` and use the high-speed formula.

Common standard-speed BRG values at 64 MHz:

| Baud rate | BRG |
|-----------|-----|
| 9600 | 415 |
| 19200 | 207 |
| 38400 | 103 |
| 57600 | 68 |
| 115200 | 34 |

## Transmit API

Queue one character on a specific UART:

```c
UART_WriteChar(&console_uart, 'A');
```

`UART_WriteChar()` blocks if the selected TX ring buffer is full, matching the
behavior expected by `printf()`.

## Receive API

```c
char ch;

if (UART_RxAvailable(&console_uart) > 0U)
{
  if (UART_ReadChar(&console_uart, &ch))
  {
    /* process ch */
  }
}
```

If the UART handle is closed, `UART_ReadChar()` returns `false` and
`UART_RxAvailable()` returns `0`.

## Flat Interrupt Wiring

In flat interrupt mode, call the generic handlers with the correct handle for
each UART instance you use:

```c
void __interrupt() ISR(void)
{
  UART_HandleRxInterrupt(&console_uart);
  UART_HandleTxInterrupt(&console_uart);
}
```

If the application uses multiple UARTs, call both handlers for each instance.

## Vectored Interrupt Wiring

Declare UART RX/TX vectors in the application and call the generic UART handlers:

```c
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
{
  UART_HandleRxInterrupt(&console_uart);
}

void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
{
  UART_HandleTxInterrupt(&console_uart);
}
```

All ISR declarations remain in application code. The library only provides
`UART_HandleRxInterrupt()` and `UART_HandleTxInterrupt()` helper functions.

## printf Routing

The XC8 runtime provides one global `putch()` hook, so only one UART can be the
active `printf()` target at a time. Select that target explicitly:

```c
UART_SelectPrintfTarget(&console_uart);
printf("Hello from UART1\r\n");
```

## Public API Summary

| Function | Purpose |
|----------|---------|
| `UART_Open()` | Opens the UART instance, applies handle configuration, and enables RX interrupts |
| `UART_Close()` | Closes the UART instance so handle configuration can be safely changed |
| `UART_WriteChar()` | Queues one character into the selected UART transmit buffer |
| `UART_ReadChar()` | Removes one character from the selected UART receive buffer |
| `UART_RxAvailable()` | Returns buffered receive bytes, or `0` when the UART is closed |
| `UART_HandleRxInterrupt()` | Services one receive interrupt for the supplied UART handle |
| `UART_HandleTxInterrupt()` | Services one transmit interrupt for the supplied UART handle |
| `UART_SelectPrintfTarget()` | Selects which initialized UART instance `putch()` and `printf()` should use |
| `putch()` | XC8 runtime hook used by `printf()`, `puts()`, and `putchar()` |

## Integration Checklist

Use this checklist when adding the library to a non-UART project:

1. Add `uartlib.h` and `uartlib.o` to the MPLAB X project.  Alternatively, you can add 
   the UARTLIB as a dependent library project. 
2. Define TX and RX buffers with power-of-2 sizes.
3. Create and fill one `uart_handle_t` for the desired UART peripheral.
4. Configure TRIS and PPS in the application for that UART instance.
5. Call `UART_Open()` during startup.
6. Enable global interrupts in the application.
7. Route RX and TX interrupts in application-owned ISRs using the generic handlers.
8. For each UART instance, ensure its ISR passes the matching `uart_handle_t`.
9. Call `UART_SelectPrintfTarget()` if the project wants to use `printf()` for debug output.

## Notes

- The library does not modify `INTCON0bits.GIE`, `GIEL`, or `GIEH`.
- Null bytes (`0x00`) received during line startup are discarded to avoid common power-up glitches 
  filling the receive buffer.
- Send/receive calls on a closed UART are ignored safely: `UART_WriteChar()` and
  `UART_ReadChar()` return `false`, and `UART_RxAvailable()` returns `0`.
