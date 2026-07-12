/* *****************************************************************************************
 *   File Name: lcd.c
 *   Description: Source file for 8-bit LCD interface functions.
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

#include <xc.h>
#include <stdbool.h>
#include <stdio.h>
#include "string.h"
#include "stdarg.h"
#include "config.h"
#include "lcd.h"

static uint8_t LCD_LINE_ADDRESSES[] = {LCD_LINE_1_ADDR, LCD_LINE_2_ADDR, 
    LCD_LINE_3_ADDR, LCD_LINE_4_ADDR};

/// @brief This function reads the busy flag and returns true if the LCD is busy,
/// or false if it is ready to accept the next command or data byte. This function
/// must be called before every command or data write to ensure the LCD is ready, and
/// it must also be called after initialization before the first command or data write.
/// In order to read the busy flag, the LCD data pins are temporarily reconfigured as inputs,
/// and the RW pin is set to read mode. After reading the busy flag, the data pins are
/// reconfigured as outputs and the RW pin is set back to write mode.
/// @param  None
/// @return true if the LCD is busy, false if it is ready for the next command or data byte
static bool LCD_ReadBusyFlag(void)
{
    LCD_DATA_TRIS = 0xFF; // Configure data pins as inputs
    LCD_RS = 0;           // Set RS low for command mode
    LCD_RW = 1;           // Set RW to read mode
    LCD_E = 1;            // Set E high to latch the busy flag on the data pins
    __delay_us(1);        // Short delay to allow data to stabilize

    bool busy = LCD_BUSY_FLAG; // Read the busy flag from the data pins (D7)

    LCD_E = 0;            // Set E low to complete the read cycle
    LCD_RW = 0;           // Set RW back to write mode
    LCD_DATA_TRIS = 0x00; // Configure data pins as outputs
    return busy;
}

/// @brief  This self test exercises the LCD by performing a series of operations
/// that demonstrate basic functionality. It clears the display, moves the cursor
/// around, prints upper and lower case alphabets, symbols, and numbers.  It also
/// demonstrates some of the basic capabilities of the HD44780 controller by performing
/// display shifts, cursor movements, and other operations. It also
/// demonstrates the backlight control function by toggling the backlight on and
/// off during the test.
/// @param  None
/// @return None
void LCD_SelfTest(void)
{
    LCD_Clear();
    LCD_SetCursor(0, 0);
    for (int i = 0; i < 20; i++)
    {
        LCD_SetCursor(0, i); // Move cursor to column i of the first line
        __delay_ms(500);
        LCD_SetCursor(2, 0); // Display the current column number on the second line
        LCD_Printf("Col: %d", i);
    }

    LCD_Clear();
    LCD_StartAtLine(LCD_LINE_1); // Move cursor to the beginning of the first line
    LCD_Print("LCD Self Test");
    LCD_StartAtLine(LCD_LINE_2); // Move cursor to the beginning of the second line
    LCD_Print("abcdefghijklmnopqrst");
    LCD_StartAtLine(LCD_LINE_3); // Move cursor to the beginning of the third line
    LCD_Print("uvwxyz");
    __delay_ms(2000);
    LCD_StartAtLine(LCD_LINE_2); // Move cursor to the beginning of the third line
    LCD_Print("ABCDEFGHIJKLMNOPQRST");
    LCD_StartAtLine(LCD_LINE_3); // Move cursor to the beginning of the third line
    LCD_Print("UCWXYZ");
    __delay_ms(2000);
    LCD_StartAtLine(LCD_LINE_2); // Move cursor to the beginning of the second line
    LCD_Print("12345678901234567890");
    LCD_StartAtLine(LCD_LINE_3); // Move cursor to the beginning of the third line
    LCD_Print("!@#$%^&*()_+-=~`");
    __delay_ms(2000);
    LCD_ClearLine(LCD_LINE_2);    // Clear the second line
    LCD_ClearLine(LCD_LINE_3);    // Clear the third line too
    LCD_StartAtLine(LCD_LINE_4);  // Move cursor to the beginning of the fourth line
    LCD_Print("Line 4");          // Print on fourth line
    __delay_ms(2000);

    LCD_Clear();
    LCD_SetCursor(0, 5);
    int pad = 10 - strlen("Centered")/2; 
    LCD_Printf("%*s", pad, "Centered");
    __delay_ms(2000);

    LCD_Clear();
    LCD_StartAtLine(LCD_LINE_4);
    LCD_Printf("%20s", "Right");
    __delay_ms(2000);
}

/// @brief Set the DDRAM address to the beginning of the specified line.
/// This is a helper function that simplifies moving the cursor to the beginning
/// of a specific line. It takes a line number (0-based index) as an argument
/// and sends the appropriate command to set the DDRAM address to the start of
/// that line.  All subsequent data writes will then start from that line until
/// the next command is sent that changes the DDRAM address.
/// @param line The 0-based index of the line to move the cursor to (0 for first
/// line, 1 for second line, etc.)
void LCD_StartAtLine(uint8_t line)
{
    uint8_t address;
    if (line < 0 || line > 3)
    {
        return; // Invalid line number, do nothing
    }
    LCD_SendCommand(LCD_CMD_SET_DDRAM_ADDR | LCD_LINE_ADDRESSES[line]);
}

/// @brief  Clear all characters on the specified line by writing spaces to 
/// every position on the line.  Leaves the cursor at the beginning of the line 
/// after clearing. This is a helper function that simplifies clearing a specific line.
/// @param line The 0-based index of the line to clear (0 for first line, 1 for 
/// second line, etc.). If an invalid line number is passed, the function does nothing.
void LCD_ClearLine(uint8_t line)
{
    if (line < 0 || line > 3)
    {
        return; // Invalid line number, do nothing
    }
    LCD_StartAtLine(line); // Move cursor to the beginning of the specified line
    for (int i = 0; i < 20; i++)
    {
        LCD_SendData(' '); // Write a space character to clear each position on the line
    }
    LCD_StartAtLine(line); // Move cursor back to the beginning of the line after clearing
}

/// @brief  Control the LCD backlight. The backlight is connected to RB2, which is
/// configured as a digital output.  This output pin drives a transistor that controls
/// power to the LCD backlight, so setting the pin high turns on the backlight and
/// setting it low turns it off.
/// @param state  true to turn on the backlight, false to turn it off
void LCD_BackLight(bool state)
{
    if (state)
    {
        LATBbits.LATB2 = 1; // Turn on backlight
    }
    else
    {
        LATBbits.LATB2 = 0; // Turn off backlight
    }
}

/// @brief  Initialize the LCD display. This function must be called before any other LCD functions are used.
/// @param  None
/// @return None
void LCD_Init(void)
{
    // Wait for LCD to power up and become ready (busy flag will be high)
    while (LCD_ReadBusyFlag())
        ;

    // Function set: 8-bit mode, 2 lines, 5x8 dots
    LCD_SendCommand(LCD_CMD_FUNCTION_SET | LCD_8_BIT_MODE | LCD_2_LINE | LCD_5x10_DOTS);
    while (LCD_ReadBusyFlag())
        ;

    // Display control: display on, cursor on, blink off
    LCD_SendCommand(LCD_CMD_DISPLAY_CONTROL | LCD_DISPLAY_ON | LCD_CURSOR_ON);
    while (LCD_ReadBusyFlag())
        ;

    // Clear display
    LCD_Clear();

    // Entry mode set: increment cursor, no display shift
    LCD_SendCommand(LCD_CMD_ENTRY_MODE_SET | LCD_ENTRY_MODE_INCREMENT);
    while (LCD_ReadBusyFlag())
        ;
}

/// @brief  Send a command to the LCD.
/// @param  cmd  The command byte to send to the LCD
/// @return None
void LCD_SendCommand(uint8_t cmd)
{
    while (LCD_ReadBusyFlag())
        ;                // Wait until LCD is not busy
    LCD_RS = 0;          // Set RS low for command mode
    LCD_RW = 0;          // Set RW low for write mode
    LCD_DATA_PORT = cmd; // Put the command byte on the data pins
    LCD_E = 1;           // Set E high to latch the command
    __delay_us(1);       // Short delay to allow data to be latched
    LCD_E = 0;           // Set E low to complete the write cycle
}

/// @brief  Send a data byte to the LCD.
/// @param  data  The data byte to send to the LCD
/// @return None
void LCD_SendData(uint8_t data)
{
    while (LCD_ReadBusyFlag())
        ;                 // Wait until LCD is not busy
    LCD_RS = 1;           // Set RS high for data mode
    LCD_RW = 0;           // Set RW low for write mode
    LCD_DATA_PORT = data; // Put the data byte on the data pins
    LCD_E = 1;            // Set E high to latch the data
    __delay_us(1);        // Short delay to allow data to be latched
    LCD_E = 0;            // Set E low to complete the write cycle
}

/// @brief  Clear the LCD display and return the cursor to the home
/// position (0,0).
/// @param  None
/// @return None
void LCD_Clear(void)
{
    LCD_SendCommand(LCD_CMD_CLEAR_DISPLAY);
    __delay_ms(2); // Clear command takes longer time to execute
}

/// @brief  Set the cursor position on the LCD.   The HD44780 controller
/// is defined for 1 or 2 rows of display data, and maps the memory addresses 
/// accordingly.  However, if the display has more than 2 lines, the additional 
/// lines are typically mapped to the memory addresses in excess of the first 2 
/// lines.  For example, on a 20x4 LCD, the first line is mapped to addresses 
/// 0x00-0x13, the second line is mapped to addresses 0x40-0x53, the third line 
/// is mapped to addresses 0x14-0x27, and the fourth line is mapped to addresses
/// 0x54-0x67.  This causes some confusion on the display when setting the cursor
/// position via the HD44780 command code.  So, what this function does instead 
/// is take a row and column number as arguments, and then it calculates the
/// correct DDRAM address based on the row and column, and sends the appropriate
/// command to set the DDRAM address.  This allows the caller to simply specify
/// the row and column they want to move the cursor to, without having to worry 
/// about how the LCD maps its memory addresses to the display lines.
/// @param  row  The row number (1-based index, 1-4 for a 4-line display)
/// @param  col  The column number (1-based index, 1-20 for a 20-column display)
/// @return None
void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address = (row == 0) ? col : (0x40 + col);
    LCD_SendCommand(LCD_CMD_SET_DDRAM_ADDR | address);
}

/// @brief  Print a string on the LCD. The string will be printed
/// starting at the current cursor position. The cursor will
/// automatically move to the right after each character is printed.
/// @param str  The string to print
/// @return None
void LCD_Print(const char *str)
{
    while (*str)
    {
        LCD_SendData((uint8_t)(*str)); // Send each character as data
        str++;                         // Move to the next character in the string
    }
}

/// @brief  Print a formatted string on the LCD. This function takes a
/// format string and additional arguments, formats the string using
/// vsnprintf, and then prints it on the LCD.
/// @param format The format string
/// @param ... Additional arguments for the format string
/// @return None
void LCD_Printf(const char *format, ...)
{
    char buffer[64]; // Buffer to hold the formatted string
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args); // Format the string
    va_end(args);
    LCD_Print(buffer); // Print the formatted string on the LCD
}
