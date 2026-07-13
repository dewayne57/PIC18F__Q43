/* *******************************************************************************
 *   File Name: hd44780.h
 *   Description: Header file for HD44780 LCD interface functions.
 *   Author: Dewayne Hafenstein
 *   Date: 2026-06-04
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
 ***************************************************************************************** */
#ifndef HD44780_H  
#define HD44780_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef LCD_WEAK
#define LCD_WEAK __attribute__((weak))
#endif

// The following definitions specify the interface connection type to the LCD.
typedef enum
{
    LCD_INTERFACE_8_BIT = 0,
    LCD_INTERFACE_4_BIT = 1
} LCD_INTERFACE_MODE;

// A single mapped MCU pin used by the LCD driver.
// - lat:  register used to drive output state
// - port: register used to read input state
// - tris: register used to set input/output direction
// - bit:  bit position 0..7 inside those registers
typedef struct
{
    volatile uint8_t *lat;
    volatile uint8_t *port;
    volatile uint8_t *tris;
    uint8_t bit;
} lcd_pin_t;

// Define a structure that represents the LCD display, including its dimensions and the port
// and pin definitions for the control and data lines.  This structure can be used to encapsulate
// all of the information about the LCD and make it easier to manage, especially if you
// want to support multiple LCDs with different configurations.  The functions in lcd.c can be
// modified to take a pointer to this structure as an argument, and then use the information in
// the structure to control the LCD.  This allows for greater flexibility and modularity in the code.
typedef struct
{
    LCD_INTERFACE_MODE interface_mode; // 8-bit or 4-bit mode
    uint8_t num_rows;                  // Number of rows on the LCD (e.g. 2 or 4)
    uint8_t num_cols;                  // Number of columns on the LCD (e.g. 16 or 20)
    uint8_t *buffer;                   // Pointer to a buffer for storing the current display contents
    size_t buffer_size;                // Size of the display buffer (should be at least num_rows * num_cols)

    // Logical LCD data pins D0..D7. In 4-bit mode only D4..D7 are used.
    lcd_pin_t data[8];

    // Control pins.
    lcd_pin_t rs;
    lcd_pin_t rw;
    lcd_pin_t e;
    lcd_pin_t backlight;
 
    // The following fields should not be modified by the user.
    uint8_t line_addresses[4]; // Array to store the DDRAM addresses for the start of each line
    bool initialized;          // Flag to indicate whether the LCD has been initialized
} lcd_handle_t;

// Define the DDRAM addresses for the start of each line for a maximum size HD44780 LCD.
// These addresses are used to calculate the correct DDRAM address when setting the cursor
// position based on the row and column.  The first line starts at address 0x00, the second
// line starts at address 0x40, the third line starts at address 0x14, and the fourth line
// starts at address 0x54.
#define LCD_LINE_1_ADDR 0x00
#define LCD_LINE_2_ADDR 0x40
#define LCD_LINE_3_ADDR 0x14
#define LCD_LINE_4_ADDR 0x54

// Define LCD command codes
#define LCD_CMD_CLEAR_DISPLAY 0x01
#define LCD_CMD_RETURN_HOME 0x02
#define LCD_CMD_ENTRY_MODE_SET 0x04
#define LCD_CMD_DISPLAY_CONTROL 0x08
#define LCD_CMD_CURSOR_SHIFT 0x10
#define LCD_CMD_FUNCTION_SET 0x20
#define LCD_CMD_SET_CGRAM_ADDR 0x40
#define LCD_CMD_SET_DDRAM_ADDR 0x80

// Define LCD command "modifiers" that are added to the base command codes above

// Used with LCD_CMD_ENTRY_MODE_SET
#define LCD_ENTRY_MODE_INCREMENT 0x02
#define LCD_ENTRY_MODE_SHIFT 0x01
// Used with LCD_CMD_DISPLAY_CONTROL
#define LCD_DISPLAY_ON 0x04
#define LCD_CURSOR_ON 0x02
#define LCD_BLINK_ON 0x01
// Used with LCD_CMD_CURSOR_SHIFT
#define LCD_CURSOR_MOVE 0x00
#define LCD_DISPLAY_MOVE 0x08
#define LCD_MOVE_RIGHT 0x04
#define LCD_MOVE_LEFT 0x00
// Used with LCD_CMD_FUNCTION_SET
#define LCD_8_BIT_MODE 0x10
#define LCD_2_LINE 0x08
#define LCD_5x10_DOTS 0x04

// The following functions can be overridden by the user to provide custom
// implementations for sending commands and data, and reading the busy flag.
// This allows the user to customize the low-level interface with the LCD,
// for example if they want to use a different pinout or a different method
// of communication (e.g. I2C or SPI instead of parallel).  The higher level
// functions like LCD_Print and LCD_SetCursor will call these lower level
// functions, so by overriding them, you can change how all of the LCD
// functions work without having to modify the higher level code. 
// 
// Note, you may want to override the LCD_BackLight function as well if 
// your LCD has a backlight control pin. By default, the LCD_Backlight 
// function checks if the backlight pin is not null and drives that pin 
// high or low based on the desired state. 
LCD_WEAK void LCD_SendCommand(lcd_handle_t *lcd, uint8_t cmd);
LCD_WEAK void LCD_SendData(lcd_handle_t *lcd, uint8_t data);
LCD_WEAK bool LCD_ReadBusyFlag(lcd_handle_t *lcd);

// The following are the main functions for controlling the LCD.  These functions
// provide a convenient interface for initializing the LCD, clearing the display,
// setting the cursor position, and printing text.  The LCD_SelfTest function is a
// demonstration function that shows how to use the other functions to perform a
// series of tests on the LCD to verify that it is working correctly.
void LCD_SelfTest(lcd_handle_t *lcd);
LCD_WEAK void LCD_BackLight(lcd_handle_t *lcd, bool state);
bool LCD_Init(lcd_handle_t *lcd);
void LCD_Clear(lcd_handle_t *lcd);
void LCD_SetCursor(lcd_handle_t *lcd, uint8_t row, uint8_t col);
void LCD_ClearLine(lcd_handle_t *lcd, uint8_t line);
void LCD_Print(lcd_handle_t *lcd, const char *str);
void LCD_PrintAt(lcd_handle_t *lcd, uint8_t row, uint8_t col, const char *str);
void LCD_Printf(lcd_handle_t *lcd, const char *format, ...);
void LCD_PrintfAt(lcd_handle_t *lcd, uint8_t row, uint8_t col, const char *format, ...);

#endif /* HD44780_H */