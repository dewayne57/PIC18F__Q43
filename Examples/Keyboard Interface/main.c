/* *****************************************************************************************
 *   File Name: main.c
 *   Description: Hardware I2C module host implementation for PIC18F47Q43.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-19
 *
 *   Copyright (c) 2026, Dewayne Hafenstein.
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 ***************************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include "config.h"
#include "i2c.h"
#include "ditto.h"
#include "ws2812.h"

#include "../../common/mcp23x17.h"
#include "../../Libraries/DMALIB/dmalib.h"
#include "../../Libraries/PPSLIB/pps.h"
#include "../../Libraries/UARTLIB/uartlib.h"
// #include "../../Libraries/I2CLIB/i2clib.h"

static I2C_Status_t MCP23017_Initialize(void);

static char console_tx_buffer[128];
static char console_rx_buffer[128];
static WS2812_Strip_t keyboard_strip;

#define KEYBOARD_LED_ROWS 5U
#define KEYBOARD_LED_COLS 6U
#define KEYBOARD_LED_COUNT (KEYBOARD_LED_ROWS * KEYBOARD_LED_COLS)
#define WS2812_ENABLE_SCOPE_SELF_TEST 0U
#define WS2812_BUSY_RECOVERY_THRESHOLD 100000UL

uart_handle_t console_uart = {.port = UART_PORT_1,
                              .high_speed_baud = false,
                              .baud_rate = 57600U,
                              .fosc = _XTAL_FREQ,
                              .data_bits = 8U,
                              .parity = UART_PARITY_NONE,
                              .stop_bits = UART_STOP_BITS_1,
                              .flow_control = UART_FLOW_NONE,
                              .tx_pin = UART_PPS_PIN_RB0,
                              .rx_pin = UART_PPS_PIN_RB1,
                              .rts_pin = UART_PPS_PIN_NONE,
                              .cts_pin = UART_PPS_PIN_NONE,
                              .isr_mode = UART_ISR_VECTORED,
                              .tx_buffer = console_tx_buffer,
                              .tx_buffer_size = sizeof(console_tx_buffer),
                              .rx_buffer = console_rx_buffer,
                              .rx_buffer_size = sizeof(console_rx_buffer),
                              .tx_head = 0U,
                              .tx_tail = 0U,
                              .rx_head = 0U,
                              .rx_tail = 0U,
                              .initialized = false};

// @brief I2C host handle for the demonstration project.
I2C_Handle_t i2c_host;
uint8_t i2c_buffer[16];                     // Buffer for I2C transactions
volatile uint8_t s_last_porta_value = 0x00; // Last known value of MCP23017 Port A
volatile bool scan_needed = false;          // Flag indicating that a scan of the keyboard
// is needed due to an external interrupt.

// @brief this table of scan codes is used to map the row and column of the keyboard
// matrix to a specific key code.  The scan code is found in the table and the corresponding
// index is used to look up the key name in the key_names array.
const uint8_t scan_codes[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45};

// @brief this table of key names is used to map the scan code to a specific key name.
const char *key_names[] = {
    "Key 00",
    "Key 01",
    "Key 02",
    "Key 03",
    "Key 04",
    "Key 05",
    "Key 10",
    "Key 11",
    "Key 12",
    "Key 13",
    "Key 14",
    "Key 15",
    "Key 20",
    "Key 21",
    "Key 22",
    "Key 23",
    "Key 24",
    "Key 25",
    "Key 30",
    "Key 31",
    "Key 32",
    "Key 33",
    "Key 34",
    "Key 35",
    "Key 40",
    "Key 41",
    "Key 42",
    "Key 43",
    "Key 44",
    "Key 45",
};

/// @brief Converts a scan code to the corresponding LED index in the
/// keyboard_strip.colors array.
/// @param scan_code The key scan code.
/// @return  The led index.
static uint8_t scanCodeToLedIndex(uint8_t scan_code)
{
    uint8_t row = (uint8_t)(scan_code >> 4U);
    uint8_t col = (uint8_t)(scan_code & 0x0FU);

    if ((row >= KEYBOARD_LED_ROWS) || (col >= KEYBOARD_LED_COLS))
    {
        return 0xFFU;
    }

    return (uint8_t)((row * KEYBOARD_LED_COLS) + col);
}

/// @brief Turns all LEDs in the keyboard_strip to off (0, 0, 0).
static void keyboardLEDsAllOff(WS2812_Strip_t *strip)
{
    for (uint8_t i = 0U; i < KEYBOARD_LED_COUNT; i++)
    {
        WS2812_SetColor(strip, i, (WS2812_Color_t){0U, 0U, 0U});
    }
    WS2812_Status_t status = WS2812_Update(strip);
    if (status != WS2812_OK)
    {
        printf("Failed to update WS2812 strip: %d%s", status, CRLF);
    }
    while ((status = WS2812_isBusy(strip)) == WS2812_BUSY)
    {
        // Wait for the WS2812 strip to become idle
    }
}

/// @brief Scan the keyboard matrix connected to the MCP23017 I/O expander.
/// @param pressed_keys A 16 element array to hold the scan codes of the
/// pressed keys.  Only the elements from 0 to num_pressed_keys-1 will be
/// valid.  The rest of the array will be unchanged.
/// @param num_pressed_keys The number of pressed keys detected (max 16).

void scanKeyboard(uint8_t *pressed_keys, uint8_t *num_pressed_keys)
{
    *num_pressed_keys = 0; // Reset the count of pressed keys
    I2C_Status_t status;

    // Mask INT1 so column transitions during scanning do not queue
    // spurious extra scan requests through the ISR.
    PIE6bits.INT1IE = 0;

    // Settle delay: wait for any keys pressed within a short window of the
    // first key to physically land before scanning the matrix.  Without this,
    // a chord pressed over ~5-30 ms (normal human timing) only catches the
    // first key because the scan completes before the other contacts close.
    __delay_ms(20);

    for (uint8_t col = 0; col < 6; col++)
    {
        // Set the current column low and all others high
        uint8_t col_value = (uint8_t)~(1 << col); // Active low for the current column
        i2c_buffer[0] = BANKED_GPIOB;
        i2c_buffer[1] = col_value;
        I2C_Write(&i2c_host, MCP23017_ADDR, i2c_buffer, 2);
        while ((status = I2C_IsBusy(&i2c_host)) == I2C_BUSY)
        {
            // Wait for the I2C bus to become idle
        }
        if (status != I2C_OK)
        {
            printf("I2C bus error after writing to GPIOB: %d%s", status, CRLF);
            return;
        }

        // Read the row inputs from Port A
        I2C_ReadRegister(&i2c_host, MCP23017_ADDR, BANKED_GPIOA, (uint8_t *)&s_last_porta_value, 1U);
        while ((status = I2C_IsBusy(&i2c_host)) == I2C_BUSY)
        {
            // Wait for the I2C bus to become idle
        }
        if (status != I2C_OK)
        {
            printf("I2C bus error after reading GPIOA: %d%s", status, CRLF);
            return;
        }

        // Check each row for a pressed key (active low)
        for (uint8_t row = 0; row < 5; row++)
        {
            if (!(s_last_porta_value & (1 << row))) // If the row input is low
            {
                uint8_t scan_code = scan_codes[row * 6 + col];
                pressed_keys[*num_pressed_keys] = scan_code;
                (*num_pressed_keys)++;

                if (*num_pressed_keys >= 16)
                {
                    return; // Stop if we have detected the maximum number of keys
                }
            }
        }
    }

    // Restore all columns to idle-low so the MCP23017 can detect the
    // next key press via interrupt-on-change.
    i2c_buffer[0] = BANKED_GPIOB;
    i2c_buffer[1] = 0x00;
    I2C_Write(&i2c_host, MCP23017_ADDR, i2c_buffer, 2);
    while (I2C_IsBusy(&i2c_host) == I2C_BUSY)
    {
    }

    // Read GPIOA to flush the MCP23017 interrupt latch (INTA de-asserts
    // only after GPIOA or INTCAPA is read, not after reading INTFA).
    I2C_ReadRegister(&i2c_host, MCP23017_ADDR, BANKED_GPIOA, i2c_buffer, 1);
    while (I2C_IsBusy(&i2c_host) == I2C_BUSY)
    {
    }

    // Re-arm PIC INT1 with a clean flag.
    PIR6bits.INT1IF = 0;
    PIE6bits.INT1IE = 1;
}

/// @brief Main application entry point.
/// @param  None
/// @return None
/// @note This application initializes the system, UART1 console output, and the I2C1 hardware
///       module host, then enters an infinite loop. The I2C host is available for I2C
///       transactions via the i2c_host handle. External interrupt on RB2 is monitored for
///       external events. The main loop remains responsive, allowing for I2C host operations
///       and UART communication.

void main(void)
{
    char rx_char;
    I2C_Status_t status;
    WS2812_Status_t ws2812_status;

    SYSTEM_Initialize();

    if (!UART_Open(&console_uart))
    {
        while (1)
        {
        }
    }

    UART_SelectPrintfTarget(&console_uart);
    printf("Keyboard Scan example%s", CRLF);
    status = I2C_Init(&i2c_host, I2C_CLOCK_MFINTOSC, I2C_MODE_MASTER_7, 0U, NONE);
    if (status != I2C_OK)
    {
        printf("I2C initialization failed with status: %d\n", status);
        while (1)
        {
        }
    }

    status = MCP23017_Initialize();
    if (status != I2C_OK)
    {
        printf("MCP23017 initialization failed with status: %d\n", status);
        while (1)
        {
        }
    }

    ws2812_status = WS2812_Init(&keyboard_strip, WS2812_PWM_MODULE_1, WS2812_PIN_RB5, KEYBOARD_LED_COUNT);
    if (ws2812_status != WS2812_OK)
    {
        printf("WS2812 initialization failed with status: %d%s", ws2812_status, CRLF);
        while (1)
        {
        }
    }

    printf("Turning all keyboard LEDs off%s", CRLF);
    keyboardLEDsAllOff(&keyboard_strip);

    // Arm INT1 only after MCP configuration and initial GPIOA read have cleared
    // any startup interrupt condition on the active-low INT line.
    IPR6bits.INT1IP = 0; // Keep INT1 on low-priority vector (matches ISR declaration).
    PIR6bits.INT1IF = 0;
    PIE6bits.INT1IE = 1;

    printf("Waiting on keyboard%s", CRLF);
    while (1)
    {
        // The keyboard scanning is performed using the MCP23017 I/O expander.  The switches are organized
        // as a matrix of rows and columns.  The rows are connected to Port A of the MCP23017, which is
        // configured as inputs with pull-ups.  The columns are connected to Port B of the MCP23017, which
        // is configured as outputs.  THe port B pins are connected via 1N4148 diodes so that when the
        // port pin is pulled low, the column is pulled low.  When High, the column floats and the
        // pull-ups on the row inputs will pull the row high.  The column lines are idled low,
        // and when a key is pressed, the corresponding row input will be pulled low.  The MCP23017 is
        // configured to generate an interrupt on the INT1 pin when any of the row inputs change state.
        // The INT1 pin is connected to the RB2/INT1 pin of the PIC18F47Q43, which is configured to
        // generate an interrupt on the falling edge.  The ISR for the INT1 interrupt sets a flag
        // indicating that a scan of the keyboard is needed.  The main loop checks this flag and performs
        // the scan when needed.
        // The keyboard scan is performed as:
        // 1. For each column, set one pin low and the others high, starting at column 0.
        // 2. Read the row inputs.  If any row input is low, a key in that column is pressed.
        // 3. The scan code is computed as (row * 6) + column, where row is the index of the low row
        //    input and column is the index of the low column output.
        // 4. More than one key press is supported, and all key scan codes are reported in the order
        //    they are detected as an array.
        // 5. Once the scan is complete, all LEDs associated with the pressed keys are set to RED
        //    and all others are turned off. The LEDs are WS2812B RGB LEDs, which are controlled by
        //    the PIC18F47Q43 using a single data line.
        // 6. As long as a scan detects any key presses, the scan is repeated.  When no keys are pressed,
        //    the scan is idle until the next external interrupt occurs, indicating that a key has been
        //    pressed or released.
        // 7. The scan code is reported to the UART console as a hexadecimal value, along with the
        //    corresponding key name.  The key names are defined in the key_names array, which is
        //    indexed by the scan code.

        // It is possible to detect key presses for specific periods of time, this would allow for the
        // detection of long presses, short presses, and double presses.  This is not implemented in
        // this example, but could be added by keeping track of the time that each key is pressed and
        // released, and then using that information to determine the type of press.  This could be
        // done by keeping track of the time that each key is pressed and released, and then using that
        // information to determine what actions are needed.  For example, a long press could be used
        // to trigger a different action than a short press, and a double press could be used to trigger
        // yet another action.  This would require additional logic to keep track of the time that each
        // key is pressed and released, and then using that information to determine what actions are
        // needed.

        if (scan_needed)
        {
            scan_needed = false;

            uint8_t pressed_keys[16];     // Array to hold the scan codes of the pressed keys
            uint8_t num_pressed_keys = 0; // Number of pressed keys detected
            char key_name[16];            // Buffer to hold the key name corresponding to the scan code

            scanKeyboard(pressed_keys, &num_pressed_keys);

            if (num_pressed_keys > 0)
            {
                printf("Number of pressed keys: %d%s", num_pressed_keys, CRLF);
                printf("Pressed keys scan codes: ");
                for (uint8_t i = 0; i < num_pressed_keys; i++)
                {
                    printf("0x%02X ", pressed_keys[i]);
                }
                printf("%s", CRLF);
                printf("Pressed keys names: ");
                for (uint8_t i = 0; i < num_pressed_keys; i++)
                {
                    for (uint8_t j = 0; j < sizeof(scan_codes) / sizeof(scan_codes[0]); j++)
                    {
                        if (pressed_keys[i] == scan_codes[j])
                        {
                            snprintf(key_name, sizeof(key_name), "%s", key_names[j]);
                            break;
                        }
                    }
                    printf("%s ", key_name);
                }
                printf("%s", CRLF);

                keyboardLEDsAllOff(&keyboard_strip);

                for (uint8_t i = 0; i < num_pressed_keys; i++)
                {
                    uint8_t led_index = scanCodeToLedIndex(pressed_keys[i]);
                    printf("LED index is %d for scan code 0x%02X%s", led_index, pressed_keys[i], CRLF);
                    WS2812_SetColor(&keyboard_strip, led_index, (WS2812_Color_t){0U, 255U, 0U});
                }
                printf("Color Buffer%s", CRLF);
                DittoDump((const uint8_t *)keyboard_strip.colors, sizeof(keyboard_strip.colors));

                if (WS2812_isBusy(&keyboard_strip) == WS2812_OK)
                {
                    printf("WS2812 strip is being updated.%s", CRLF);
                    ws2812_status = WS2812_Update(&keyboard_strip);
                    if (ws2812_status != WS2812_OK)
                    {
                        printf("WS2812 update failed with status: %d%s", ws2812_status, CRLF);
                    }
                }
                else
                {
                    printf("WS2812 strip busy, update skipped.%s", CRLF);
                }
            }
        }
    }
}

/// @brief UART1 RX ISR (vectored)
/// @param  None
/// @return None
void __interrupt(irq(IRQ_U1RX), low_priority) UART1_RX_ISR(void)
{
    UART_HandleRxInterrupt(&console_uart);
}

/// @brief UART1 TX ISR (vectored)
/// @param  None
/// @return None
void __interrupt(irq(IRQ_U1TX), low_priority) UART1_TX_ISR(void)
{
    UART_HandleTxInterrupt(&console_uart);
}

/// @brief Keyboard key has been hit (IOC occurred on RB2/INT1).  This ISR is triggered by the
/// external interrupt on RB2, which is connected to the INT1 pin of the MCP23017 I/O expander.
/// The ISR sets a flag indicating that the value of Port B has changed, allowing the main loop
/// to handle the I2C read and write operations. The ISR clears the interrupt flag for INT1 to
/// acknowledge the interrupt and allow for future interrupts to be detected. This ISR is declared
/// as a low-priority interrupt to ensure that it does not interfere with higher-priority interrupts,
/// such as UART communication.
/// @param  None
/// @return None
/// @note This simply sets a flag to indicate that the port B value has changed. The main loop
/// will handle the I2C read and write operations.
void __interrupt(irq(IRQ_INT1), low_priority) Extern_ISR(void)
{
    scan_needed = true; // Indicate that a scan of the keyboard is needed due to an external interrupt.
    PIR6bits.INT1IF = 0;
}

/// @brief Initialize the MCP23017 I/O expander.
/// The MCP23017 is configured in banked addressing mode so Port A registers are
/// contiguous within one block and Port B registers are contiguous within another.
/// @return i2c_status_t indicating success or error
static I2C_Status_t MCP23017_Initialize()
{
    uint8_t iocon = 0x84;
    I2C_Status_t status;

    PORTBbits.RB3 = 0; // reset the MCP23017
    __delay_us(100);
    PORTBbits.RB3 = 1; // Clear reset condition
    __delay_us(100);

    // Keep INT1 disabled during setup; main() enables it after initial GPIOA read.
    PIE6bits.INT1IE = 0;

    // Enter banked mode (BANK=1) while keeping address auto-increment enabled (SEQOP=0).
    // INTPOL=0 keeps INT active low; ODR=1 enables open-drain interrupt output.
    i2c_buffer[0] = IOCON;
    i2c_buffer[1] = iocon;
    status = I2C_Write(&i2c_host, MCP23017_ADDR, i2c_buffer, 2);
    if (status != I2C_OK)
    {
        return status;
    }

    while ((status = I2C_IsBusy(&i2c_host)) == I2C_BUSY)
    {
        // Wait for the I2C bus to become idle
    }
    if (status != I2C_OK)
    {
        printf("I2C bus error after writing to IOCON: %d%s", status, CRLF);
        return status;
    }

    // Enable Port A pull-ups BEFORE arming interrupt-on-change.  If GPPUA is written
    // after GPINTENA (as happens in a sequential auto-increment write that spans both
    // registers), the pull-ups transitioning the floating row pins high looks like a
    // real change to the interrupt logic and fires a spurious interrupt at startup.
    i2c_buffer[0] = BANKED_GPPUA;
    i2c_buffer[1] = 0xFF; // GPPUA: enable all pull-ups
    status = I2C_Write(&i2c_host, MCP23017_ADDR, i2c_buffer, 2);
    if (status != I2C_OK)
    {
        return status;
    }
    while ((status = I2C_IsBusy(&i2c_host)) == I2C_BUSY)
    {
        // Wait for the I2C bus to become idle
    }
    if (status != I2C_OK)
    {
        return status;
    }

    // Configure Port A in one contiguous banked transfer.
    i2c_buffer[0] = BANKED_IODIRA;
    i2c_buffer[1] = 0xFF;  // IODIRA: inputs
    i2c_buffer[2] = 0x00;  // IPOLA
    i2c_buffer[3] = 0xFF;  // GPINTENA: arm interrupt-on-change (pull-ups already active)
    i2c_buffer[4] = 0x00;  // DEFVALA
    i2c_buffer[5] = 0x00;  // INTCONA
    i2c_buffer[6] = iocon; // IOCON
    i2c_buffer[7] = 0xFF;  // GPPUA (redundant but keeps block self-consistent)

    status = I2C_Write(&i2c_host, MCP23017_ADDR, i2c_buffer, 8);
    if (status != I2C_OK)
    {
        return status;
    }
    while ((status = I2C_IsBusy(&i2c_host)) == I2C_BUSY)
    {
        // Wait for the I2C bus to become idle
    }
    if (status != I2C_OK)
    {
        printf("I2C bus error after writing to IOCON: %d%s", status, CRLF);
        return status;
    }

    // Configure Port B in one contiguous banked transfer.
    i2c_buffer[0] = BANKED_IODIRB;
    i2c_buffer[1] = 0x00;  // IODIRB: outputs
    i2c_buffer[2] = 0x00;  // IPOLB
    i2c_buffer[3] = 0x00;  // GPINTENB
    i2c_buffer[4] = 0x00;  // DEFVALB
    i2c_buffer[5] = 0x00;  // INTCONB
    i2c_buffer[6] = iocon; // IOCON
    i2c_buffer[7] = 0x00;  // GPPUB
    i2c_buffer[8] = 0x00;  // INTFB (write ignored)
    i2c_buffer[9] = 0x00;  // INTCAPB (write ignored)
    i2c_buffer[10] = 0x00; // GPIOB
    status = I2C_Write(&i2c_host, MCP23017_ADDR, i2c_buffer, 11);
    if (status != I2C_OK)
    {
        return status;
    }
    while ((status = I2C_IsBusy(&i2c_host)) == I2C_BUSY)
    {
        // Wait for the I2C bus to become idle
    }
    if (status != I2C_OK)
    {
        printf("I2C bus error after writing to IOCON: %d%s", status, CRLF);
        return status;
    }

    // Reading GPIOA is what actually clears the MCP23017 interrupt latch.
    // INTFA just reports which pin caused the interrupt; it does not de-assert INTA.
    status = I2C_ReadRegister(&i2c_host, MCP23017_ADDR, BANKED_GPIOA, i2c_buffer, 1);
    s_last_porta_value = i2c_buffer[0]; // Capture the settled startup state.

    return status;
}
