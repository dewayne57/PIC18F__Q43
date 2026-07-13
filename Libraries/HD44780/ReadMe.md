# HD44780 Library

Parallel HD44780 LCD driver for PIC18F Q43/Q84 projects. The library supports
both 8-bit and 4-bit interface modes and uses an application-owned
`lcd_handle_t` for all wiring and display configuration.

The pin mapping model is per signal, so data and control lines do not need to be
contiguous and do not need to be on a single MCU port.

## Intended Use

This library is intended as a reusable building block for projects that need
character LCD output. It is separate from one-off demo code so other examples in
this repository can share one LCD implementation.

## Files

| File    | Purpose |
| ------- | ------- |
| `lcd.h` | Public API, constants, and `lcd_handle_t` configuration types |
| `lcd.c` | HD44780 implementation (init, command/data writes, text helpers, self test) |

## Design Summary

- Application owns all LCD state through one `lcd_handle_t` per LCD instance.
- Library performs no dynamic allocation.
- User provides optional text buffer for `LCD_Printf()` and `LCD_PrintfAt()`.
- Supports:
  - `LCD_INTERFACE_8_BIT` using D0..D7.
  - `LCD_INTERFACE_4_BIT` using D4..D7 only.
- Busy-flag polling is used (`RW` must be wired if you use this implementation as-is).
- Backlight control is optional. If the backlight pin descriptor is invalid, calls are ignored safely.

## Pin Mapping Model

Each physical LCD signal is mapped with `lcd_pin_t`:

```c
typedef struct
{
	volatile uint8_t *lat;   // output latch register
	volatile uint8_t *port;  // input read register
	volatile uint8_t *tris;  // direction register
	uint8_t bit;             // bit number 0..7
} lcd_pin_t;
```

Each signal can point to any register/bit combination, allowing mixed-port or
non-contiguous wiring.

`lcd_handle_t` includes:

- `interface_mode`, `num_rows`, `num_cols`
- `data[8]` logical LCD D0..D7
- `rs`, `rw`, `e`, `backlight`
- `buffer`, `buffer_size` for formatted text helpers

## Required Configuration

Before calling `LCD_Init()`:

1. Set `interface_mode` to `LCD_INTERFACE_8_BIT` or `LCD_INTERFACE_4_BIT`.
2. Set `num_rows` and `num_cols` for your display geometry.
3. Fill required pin mappings:
   - Always required: `rs`, `rw`, `e`
   - 8-bit mode: `data[0]` through `data[7]`
   - 4-bit mode: `data[4]` through `data[7]`
4. Optionally provide `buffer` and `buffer_size` for `LCD_Printf*` functions.
5. Initialize `initialized = false` in your static handle initializer.

## 8-bit Example Handle

```c
static char lcd_text_buffer[80];

static lcd_handle_t lcd = {
	.interface_mode = LCD_INTERFACE_8_BIT,
	.num_rows = 4,
	.num_cols = 20,
	.data = {
		{&LATD, &PORTD, &TRISD, 0},
		{&LATD, &PORTD, &TRISD, 1},
		{&LATD, &PORTD, &TRISD, 2},
		{&LATD, &PORTD, &TRISD, 3},
		{&LATD, &PORTD, &TRISD, 4},
		{&LATD, &PORTD, &TRISD, 5},
		{&LATD, &PORTD, &TRISD, 6},
		{&LATD, &PORTD, &TRISD, 7},
	},
	.rs = {&LATE, &PORTE, &TRISE, 0},
	.rw = {&LATE, &PORTE, &TRISE, 1},
	.e  = {&LATE, &PORTE, &TRISE, 2},
	.backlight = {&LATB, &PORTB, &TRISB, 2},
	.buffer = lcd_text_buffer,
	.buffer_size = sizeof(lcd_text_buffer),
	.initialized = false,
};
```

## 4-bit Example (Non-contiguous Pins)

```c
static lcd_handle_t lcd4 = {
	.interface_mode = LCD_INTERFACE_4_BIT,
	.num_rows = 2,
	.num_cols = 16,
	.data = {
		{0}, {0}, {0}, {0},
		{&LATA, &PORTA, &TRISA, 5}, // D4
		{&LATC, &PORTC, &TRISC, 3}, // D5
		{&LATB, &PORTB, &TRISB, 7}, // D6
		{&LATD, &PORTD, &TRISD, 1}, // D7
	},
	.rs = {&LATE, &PORTE, &TRISE, 0},
	.rw = {&LATE, &PORTE, &TRISE, 1},
	.e  = {&LATE, &PORTE, &TRISE, 2},
	.initialized = false,
};
```

## Initialization and Basic Use

```c
if (!LCD_Init(&lcd))
{
	while (1) { }
}

LCD_BackLight(&lcd, true);
LCD_Clear(&lcd);
LCD_PrintAt(&lcd, 1, 1, "Hello");
LCD_PrintfAt(&lcd, 2, 1, "Count=%u", 42U);
```

## Public API Summary

| Function | Purpose |
| -------- | ------- |
| `LCD_Init()` | Validates handle, configures pins, runs HD44780 initialization sequence |
| `LCD_Clear()` | Clears display and returns cursor home |
| `LCD_SetCursor()` | Sets cursor by 1-based row/column |
| `LCD_Print()` | Prints null-terminated text at current cursor |
| `LCD_PrintAt()` | Moves cursor, then prints text |
| `LCD_Printf()` | Formats text and prints at current cursor |
| `LCD_PrintfAt()` | Formats text and prints at a specific position |
| `LCD_ClearLine()` | Clears one line and returns cursor to column 1 |
| `LCD_SendCommand()` | Low-level command write |
| `LCD_SendData()` | Low-level data write |
| `LCD_ReadBusyFlag()` | Low-level busy-flag read |
| `LCD_BackLight()` | Drives backlight pin if configured |
| `LCD_SelfTest()` | Built-in display exercise routine |

## Notes and Constraints

- Rows and columns are 1-based in API calls.
- `LCD_Init()` currently accepts `1..4` rows and `1..40` columns.
- `LCD_Printf*` uses the handle buffer when provided; otherwise a small internal fallback buffer is used.
- RW is actively used for busy-flag polling. If your hardware ties RW low, this implementation must be adjusted to use fixed delays instead of `LCD_ReadBusyFlag()`.
- This library includes `config.h` and relies on XC8 delay macros (`__delay_ms`, `__delay_us`) and `_XTAL_FREQ`.

## Integration Checklist

1. Add `Libraries/HD44780/lcd.c` and `Libraries/HD44780/lcd.h` to the project.
2. Ensure `_XTAL_FREQ` is defined in the project configuration.
3. Create and initialize one `lcd_handle_t` instance.
4. Call `LCD_Init(&handle)` once at startup.
5. Use `LCD_Print*` helpers for normal output.

## Extension or Adaptation

The four hardware-facing functions are declared weak so you can replace them with
your own transport implementation (for example MCP23017 over I2C, SPI shift register,
or another GPIO expander):

- `LCD_SendCommand(lcd_handle_t *lcd, uint8_t cmd)`
- `LCD_SendData(lcd_handle_t *lcd, uint8_t data)`
- `LCD_ReadBusyFlag(lcd_handle_t *lcd)`
- `LCD_BackLight(lcd_handle_t *lcd, bool state)`

### What you need to do

1. Add a new source file to your project, for example `lcd_transport_mcp23017.c`.
2. Include `lcd.h` in that file.
3. Implement all four functions above with the exact same signatures.
4. Ensure your project links your transport file along with the library.
5. In your override implementations, do not call the default library versions of those same functions.

At link time, your strong function definitions replace the weak defaults.

### Required behavior for each function

#### `LCD_SendCommand`

- Drive `RS = 0`, `RW = 0` on your transport.
- Put the command byte on the LCD bus.
- Pulse `E` high then low with timing that satisfies the HD44780 datasheet.
- In 4-bit mode, send high nibble first, then low nibble.
- If you are not using busy-flag polling, apply command execution delays here (and/or in `LCD_SendData`).

#### `LCD_SendData`

- Drive `RS = 1`, `RW = 0` on your transport.
- Put the data byte on the LCD bus.
- Pulse `E` high then low with valid setup/hold timing.
- In 4-bit mode, send high nibble first, then low nibble.
- If not polling busy, apply safe write delays for data writes.

#### `LCD_ReadBusyFlag`

- If RW is connected and readable through the transport:
	- Configure bus for read.
	- Drive `RS = 0`, `RW = 1`.
	- Read D7 and return `true` while busy.
	- In 4-bit mode, complete both read strobes (high nibble and low nibble).
- If RW is tied low or reads are impractical:
	- Return `false`.
	- Make sure `LCD_SendCommand` and `LCD_SendData` enforce conservative delays.

#### `LCD_BackLight`

- If your hardware exposes backlight control, set it on/off based on `state`.
- If backlight is fixed or not present, this function may be a no-op.

### Timing guidance when not using busy-flag reads

Typical conservative delays often used with HD44780-compatible modules:

- Most commands/data writes: around 37 us (or greater)
- `LCD_CMD_CLEAR_DISPLAY` and `LCD_CMD_RETURN_HOME`: around 1.52 ms (or greater)

Always verify against your specific LCD module datasheet and oscillator tolerance.

### Minimal validation checklist for custom transports

1. `LCD_Init()` returns success.
2. `LCD_Clear()` visibly clears the display.
3. `LCD_PrintAt()` writes expected text and positions correctly.
4. `LCD_PrintfAt()` works for your formatted strings.
5. Backlight control behaves as intended.
