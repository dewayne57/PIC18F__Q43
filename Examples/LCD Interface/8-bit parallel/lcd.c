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

#define LCD_PRINTF_FALLBACK_BUFFER_SIZE 64U

/// @brief  Check if the LCD has been initialized by checking the initialized flag in 
/// the lcd_handle_t structure. This function can be used at the beginning of each LCD 
/// function to ensure that the LCD has been initialized before attempting to send 
/// commands or data. If the LCD has not been initialized, the function will return 
/// false. If the LCD has been initialized, it will return true. This helps prevent errors
/// caused by calling LCD functions before initialization, and it also allows the LCD
/// functions to be more robust and self-contained by handling the initialization check 
/// internally.
/// @param  lcd Pointer to the lcd_handle_t structure that contains the configuration for the LCD
/// interface. This structure must be properly initialized with the correct interface mode,
/// number of rows and columns, data port and control pin configurations, and a buffer for
/// storing the current display contents. The function will check the initialized flag in this
/// structure to determine if the LCD has been initialized.
/// @return true if the LCD has been initialized, false if it has not been initialized
static bool checkIfInitialized(lcd_handle_t *lcd)
{
    if (lcd == NULL)
    {
        return false;
    }
    if (!lcd->initialized)
    {
        return false; // LCD is not initialized, return false
    }
    return true; // LCD is initialized, return true
}

static bool isPinValid(const lcd_pin_t *pin)
{
    return (pin != NULL) && (pin->lat != NULL) && (pin->port != NULL) && (pin->tris != NULL) && (pin->bit < 8U);
}

static void pinWrite(const lcd_pin_t *pin, bool level)
{
    uint8_t mask;

    if (!isPinValid(pin))
    {
        return;
    }
    mask = (uint8_t)(1U << pin->bit);
    if (level)
    {
        *(pin->lat) |= mask;
    }
    else
    {
        *(pin->lat) &= (uint8_t)(~mask);
    }
}

static bool pinRead(const lcd_pin_t *pin)
{
    uint8_t mask;

    if (!isPinValid(pin))
    {
        return false;
    }
    mask = (uint8_t)(1U << pin->bit);
    return ((*(pin->port) & mask) != 0U);
}

static void pinSetInput(const lcd_pin_t *pin, bool input)
{
    uint8_t mask;

    if (!isPinValid(pin))
    {
        return;
    }
    mask = (uint8_t)(1U << pin->bit);
    if (input)
    {
        *(pin->tris) |= mask;
    }
    else
    {
        *(pin->tris) &= (uint8_t)(~mask);
    }
}

static bool hasRequiredPins(const lcd_handle_t *lcd)
{
    uint8_t i;
    uint8_t start;

    if ((lcd == NULL) || !isPinValid(&lcd->rs) || !isPinValid(&lcd->rw) || !isPinValid(&lcd->e))
    {
        return false;
    }

    start = (lcd->interface_mode == LCD_INTERFACE_8_BIT) ? 0U : 4U;
    for (i = start; i < 8U; i++)
    {
        if (!isPinValid(&lcd->data[i]))
        {
            return false;
        }
    }

    return true;
}

static void setDataDirection(lcd_handle_t *lcd, bool input)
{
    uint8_t i;
    uint8_t start = (lcd->interface_mode == LCD_INTERFACE_8_BIT) ? 0U : 4U;
    for (i = start; i < 8U; i++)
    {
        pinSetInput(&lcd->data[i], input);
    }
}

static void writeDataBus8(lcd_handle_t *lcd, uint8_t value)
{
    uint8_t i;
    for (i = 0; i < 8U; i++)
    {
        pinWrite(&lcd->data[i], ((value & (uint8_t)(1U << i)) != 0U));
    }
}

static void writeNibble4(lcd_handle_t *lcd, uint8_t nibble)
{
    uint8_t i;
    for (i = 0; i < 4U; i++)
    {
        pinWrite(&lcd->data[i + 4U], ((nibble & (uint8_t)(1U << i)) != 0U));
    }
}

static void pulseEnable(lcd_handle_t *lcd)
{
    pinWrite(&lcd->e, true);
    __delay_us(1);
    pinWrite(&lcd->e, false);
    __delay_us(1);
}

/// @brief This function reads the busy flag and returns true if the LCD is busy,
/// or false if it is ready to accept the next command or data byte.
/// @param  lcd Pointer to the lcd_handle_t structure that contains the LCD pin mapping.
/// @return true if the LCD is busy, false if it is ready for the next command or data byte
bool LCD_ReadBusyFlag(lcd_handle_t *lcd)
{
    bool busy;

    if ((lcd == NULL) || !hasRequiredPins(lcd))
    {
        return false;
    }

    setDataDirection(lcd, true); // Configure data pins as inputs
    pinWrite(&lcd->rs, false);   // Set RS low for command mode
    pinWrite(&lcd->rw, true);    // Set RW to read mode

    if (lcd->interface_mode == LCD_INTERFACE_8_BIT)
    {
        pinWrite(&lcd->e, true);
        __delay_us(1);
        busy = pinRead(&lcd->data[7]);
        pinWrite(&lcd->e, false);
    }
    else
    {
        pinWrite(&lcd->e, true);
        __delay_us(1);
        busy = pinRead(&lcd->data[7]); // Busy bit is D7 in the high nibble
        pinWrite(&lcd->e, false);

        // Read and discard low nibble to complete the 4-bit read cycle.
        pinWrite(&lcd->e, true);
        __delay_us(1);
        pinWrite(&lcd->e, false);
    }

    pinWrite(&lcd->rw, false);   // Set RW back to write mode
    setDataDirection(lcd, false); // Restore data pins as outputs
    return busy;
}

/// @brief  Send a command to the LCD.
/// @param  lcd Pointer to the lcd_handle_t structure that contains the configuration for 
/// the LCD interface. This structure must be properly initialized with the correct interface
/// mode, number of rows and columns, data port and control pin configurations, and a buffer 
/// for storing the current display contents. The function will use this information to send 
/// the command byte to the LCD and update the display buffer accordingly.
/// @param  cmd  The command byte to send to the LCD
/// @return None
void LCD_SendCommand(lcd_handle_t *lcd, uint8_t cmd)
{
    if ((lcd == NULL) || !hasRequiredPins(lcd))
    {
        return;
    }

    if (lcd->initialized)
    {
        while (LCD_ReadBusyFlag(lcd))
            ; // Wait until LCD is not busy
    }

    pinWrite(&lcd->rs, false); // Set RS low for command mode
    pinWrite(&lcd->rw, false); // Set RW low for write mode

    if (lcd->interface_mode == LCD_INTERFACE_8_BIT)
    {
        writeDataBus8(lcd, cmd);
        pulseEnable(lcd);
    }
    else
    {
        writeNibble4(lcd, (uint8_t)(cmd >> 4));
        pulseEnable(lcd);
        writeNibble4(lcd, (uint8_t)(cmd & 0x0FU));
        pulseEnable(lcd);
    }
}

/// @brief  Send a data byte to the LCD.
/// @param  lcd Pointer to the lcd_handle_t structure that contains the configuration 
/// for the LCD interface. This structure must be properly initialized with the correct 
/// interface mode, number of rows and columns, data port and control pin configurations, 
/// and a buffer for storing the current display contents. The function will use this 
/// information to send the data byte to the LCD and update the display buffer accordingly.
/// @param  data  The data byte to send to the LCD
/// @return None
void LCD_SendData(lcd_handle_t *lcd, uint8_t data)
{
    if ((lcd == NULL) || !hasRequiredPins(lcd))
    {
        return;
    }

    if (lcd->initialized)
    {
        while (LCD_ReadBusyFlag(lcd))
            ; // Wait until LCD is not busy
    }

    pinWrite(&lcd->rs, true);  // Set RS high for data mode
    pinWrite(&lcd->rw, false); // Set RW low for write mode

    if (lcd->interface_mode == LCD_INTERFACE_8_BIT)
    {
        writeDataBus8(lcd, data);
        pulseEnable(lcd);
    }
    else
    {
        writeNibble4(lcd, (uint8_t)(data >> 4));
        pulseEnable(lcd);
        writeNibble4(lcd, (uint8_t)(data & 0x0FU));
        pulseEnable(lcd);
    }
}

/// @brief  Initialize the LCD display. This function must be called before any other LCD functions are used.
/// @param  lcd Pointer to the lcd_handle_t structure that contains the configuration for the LCD
/// interface. This structure must be properly initialized with the correct interface mode, number
/// of rows and columns, data port and control pin configurations, and a buffer for storing the
/// current display contents. The function will use this information to initialize the LCD and
/// prepare it for use.
/// @return True if the LCD was successfully initialized, or false if there was an error during
/// initialization (e.g. invalid configuration parameters).
bool LCD_Init(lcd_handle_t *lcd)
{
    uint8_t i;
    uint8_t function_set;

    if ((lcd == NULL) || !hasRequiredPins(lcd))
    {
        return false;
    }

    // Check if the LCD has already been initialized to prevent re-initialization
    if (checkIfInitialized(lcd))
    {
        return true; // LCD is already initialized, return success
    }

    // First, calculate the DDRAM addresses for the start of each line based on the number of columns and rows.
    // This allows the LCD_SetCursor function to set the correct DDRAM address based on the row and column.
    if (lcd->num_rows < 1 || lcd->num_rows > 4 || lcd->num_cols < 1 || lcd->num_cols > 40)
    {
        return false; // Invalid configuration parameters
    }
    lcd->line_addresses[0] = LCD_LINE_1_ADDR;
    lcd->line_addresses[1] = LCD_LINE_2_ADDR;
    lcd->line_addresses[2] = LCD_LINE_3_ADDR;
    lcd->line_addresses[3] = LCD_LINE_4_ADDR;

    // Configure data and control pins as outputs.
    setDataDirection(lcd, false);
    pinSetInput(&lcd->rs, false);
    pinSetInput(&lcd->rw, false);
    pinSetInput(&lcd->e, false);
    pinWrite(&lcd->rs, false);
    pinWrite(&lcd->rw, false);
    pinWrite(&lcd->e, false);

    if (isPinValid(&lcd->backlight))
    {
        pinSetInput(&lcd->backlight, false);
        pinWrite(&lcd->backlight, false);
    }

    lcd->initialized = false;
    __delay_ms(20);

    if (lcd->interface_mode == LCD_INTERFACE_8_BIT)
    {
        for (i = 0; i < 3U; i++)
        {
            writeDataBus8(lcd, 0x30);
            pulseEnable(lcd);
            __delay_ms(5);
        }
    }
    else
    {
        for (i = 0; i < 3U; i++)
        {
            writeNibble4(lcd, 0x03);
            pulseEnable(lcd);
            __delay_ms(5);
        }
        writeNibble4(lcd, 0x02);
        pulseEnable(lcd);
        __delay_ms(1);
    }

    function_set = LCD_CMD_FUNCTION_SET;
    if (lcd->interface_mode == LCD_INTERFACE_8_BIT)
    {
        function_set |= LCD_8_BIT_MODE;
    }
    if (lcd->num_rows > 1U)
    {
        function_set |= LCD_2_LINE;
    }

    LCD_SendCommand(lcd, function_set);
    lcd->initialized = true;

    // Display control: display on, cursor on, blink off
    LCD_SendCommand(lcd, LCD_CMD_DISPLAY_CONTROL | LCD_DISPLAY_ON | LCD_CURSOR_ON);

    // Clear display
    LCD_Clear(lcd);

    // Entry mode set: increment cursor, no display shift
    LCD_SendCommand(lcd, LCD_CMD_ENTRY_MODE_SET | LCD_ENTRY_MODE_INCREMENT);

    return true;
}

/// @brief  Control the LCD backlight. The backlight is connected to RB2, which is
/// configured as a digital output.  This output pin drives a transistor that controls
/// power to the LCD backlight, so setting the pin high turns on the backlight and
/// setting it low turns it off.
/// @param state  true to turn on the backlight, false to turn it off
void LCD_BackLight(lcd_handle_t *lcd, bool state)
{
    if ((lcd == NULL) || !isPinValid(&lcd->backlight))
    {
        return;
    }
    pinWrite(&lcd->backlight, state);
}

/// @brief  Clear all characters on the specified line by writing spaces to
/// every position on the line.  Leaves the cursor at the beginning of the line
/// after clearing. This is a helper function that simplifies clearing a specific line.
/// @param line The 1-based index of the line to clear (1 for first line, 2 for
/// second line, etc.). If an invalid line number is passed, the function does nothing.
void LCD_ClearLine(lcd_handle_t *lcd, uint8_t line)
{
    if (!checkIfInitialized(lcd))
    {
        return; // LCD is not initialized, do nothing
    }

    if (line < 1 || line > lcd->num_rows)
    {
        return; // Invalid line number, do nothing
    }
    for (uint8_t i = 1; i <= lcd->num_cols; i++)
    {
        LCD_PrintAt(lcd, line, i, " "); // Write a space character to clear each position on the line
    }
    LCD_SetCursor(lcd, line, 1);
}

/// @brief  Clear the LCD display and return the cursor to the home
/// position (0,0).
/// @param  None
/// @return None
void LCD_Clear(lcd_handle_t *lcd)
{
    if (!checkIfInitialized(lcd))
    {
        while (1)
            ; // LCD is not initialized, enter infinite loop to prevent further execution
    }
    LCD_SendCommand(lcd, LCD_CMD_CLEAR_DISPLAY);
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
/// @param  lcd Pointer to the lcd_handle_t structure that contains the configuration
/// for the LCD interface. This structure must be properly initialized with the 
/// correct interface mode, number of rows and columns, data port and control 
/// pin configurations, and a buffer for storing the current display contents. 
/// The function will use this information to calculate the correct DDRAM address
/// based on the specified row and column, and then send the appropriate command
/// to set the cursor position on the LCD.
/// @param  row  The row number (1-based index, 1-4 for a 4-line display)
/// @param  col  The column number (1-based index, 1-20 for a 20-column display)
/// @return None
void LCD_SetCursor(lcd_handle_t *lcd, uint8_t row, uint8_t col)
{
    if (row < 1 || row > lcd->num_rows || col < 1 || col > lcd->num_cols)
    {
        return; // Invalid row or column, do nothing
    }
    // Calculate DDRAM address based on row and column
    uint8_t address = lcd->line_addresses[row - 1] + (col - 1);
    LCD_SendCommand(lcd, LCD_CMD_SET_DDRAM_ADDR | address);
}

/// @brief  Print a string on the LCD. The string will be printed
/// starting at the current cursor position. The cursor will
/// automatically move to the right after each character is printed.
/// @param lcd Pointer to the lcd_handle_t structure that contains the configuration
/// for the LCD interface. This structure must be properly initialized with the 
/// correct interface mode, number of rows and columns, data port and control pin 
/// configurations, and a buffer for storing the current display contents. The 
/// function will use this information to send each character of the string to the 
/// LCD as data, and it will also update the display buffer accordingly. The 
/// function will continue to print characters until the end of the string is reached.
/// @param str  The string to print
/// @return None
void LCD_Print(lcd_handle_t *lcd, const char *str)
{
    while (*str)
    {
        LCD_SendData(lcd, (uint8_t)(*str)); // Send each character as data
        str++;                         // Move to the next character in the string
    }
}

/// @brief  Print a formatted string on the LCD. This function takes a
/// format string and additional arguments, formats the string using
/// vsnprintf, and then prints it on the LCD.
/// @param lcd Pointer to the lcd_handle_t structure that contains the configuration
/// for the LCD interface. This structure must be properly initialized with the 
/// correct interface mode, number of rows and columns, data port and control pin 
/// configurations, and a buffer for storing the current display contents. The 
/// function will use this information to send each character of the string to the 
/// LCD as data, and it will also update the display buffer accordingly. The 
/// function will continue to print characters until the end of the string is reached.
/// @param format The format string
/// @param ... Additional arguments for the format string
/// @return None
void LCD_Printf(lcd_handle_t *lcd, const char *format, ...)
{
    char temp[LCD_PRINTF_FALLBACK_BUFFER_SIZE];
    char *dst;
    size_t dst_size;

    if (!checkIfInitialized(lcd))
    {
        return;
    }

    if ((lcd->buffer != NULL) && (lcd->buffer_size > 0U))
    {
        dst = (char *)lcd->buffer;
        dst_size = lcd->buffer_size;
    }
    else
    {
        dst = temp;
        dst_size = sizeof(temp);
    }

    va_list args;
    va_start(args, format);
    (void)vsnprintf(dst, dst_size, format, args);
    va_end(args);
    LCD_Print(lcd, dst); // Print the formatted string on the LCD
}

/// @brief  Print a string on the LCD at a specified position. This function
/// moves the cursor to the specified row and column, and then prints the
/// string starting from that position. The cursor will automatically move
/// to the right after each character is printed, and if the string exceeds
/// the end of the line, it will wrap to the next line. This function is a
/// convenient way to print text at a specific location on the display without
/// having to manually set the cursor position before calling LCD_Print().
/// @param lcd Pointer to the lcd_handle_t structure that contains the configuration
/// for the LCD interface. This structure must be properly initialized with the 
/// correct interface mode, number of rows and columns, data port and control pin 
/// configurations, and a buffer for storing the current display contents. The 
/// function will use this information to send each character of the string to the 
/// LCD as data, and it will also update the display buffer accordingly. The 
/// function will continue to print characters until the end of the string is reached.
/// @param row The row number (1-based) where the string should start
/// @param col The column number (1-based) where the string should start
/// @param str The string to print
void LCD_PrintAt(lcd_handle_t *lcd, uint8_t row, uint8_t col, const char *str)
{
    LCD_SetCursor(lcd, row, col); // Move cursor to the specified position
    LCD_Print(lcd, str);          // Print the string at the specified position
}

/// @brief  Print a formatted string on the LCD at a specified position. This
/// function moves the cursor to the specified row and column, formats the string
/// using vsnprintf, and then prints it on the LCD. The cursor will automatically
/// move to the right after each character is printed, and if the string exceeds
/// the end of the line, it will wrap to the next line. This function is a
/// convenient way to print formatted text at a specific location on the display
/// without having to manually set the cursor position before calling LCD_Printf().
/// @param lcd Pointer to the lcd_handle_t structure that contains the configuration
/// for the LCD interface. This structure must be properly initialized with the 
/// correct interface mode, number of rows and columns, data port and control pin 
/// configurations, and a buffer for storing the current display contents. The 
/// function will use this information to send each character of the string to the 
/// LCD as data, and it will also update the display buffer accordingly. The 
/// function will continue to print characters until the end of the string is reached.
/// @param row The row number (1-based) where the formatted string should start
/// @param col The column number (1-based) where the formatted string should start
/// @param format The format string
/// @param ... Additional arguments for the format string
void LCD_PrintfAt(lcd_handle_t *lcd, uint8_t row, uint8_t col, const char *format, ...)
{
    char temp[LCD_PRINTF_FALLBACK_BUFFER_SIZE];
    char *dst;
    size_t dst_size;

    if (!checkIfInitialized(lcd))
    {
        return;
    }

    if ((lcd->buffer != NULL) && (lcd->buffer_size > 0U))
    {
        dst = (char *)lcd->buffer;
        dst_size = lcd->buffer_size;
    }
    else
    {
        dst = temp;
        dst_size = sizeof(temp);
    }

    va_list args;
    va_start(args, format);
    (void)vsnprintf(dst, dst_size, format, args);
    va_end(args);
    LCD_SetCursor(lcd, row, col); // Move cursor to the specified position
    LCD_Print(lcd, dst);          // Print the formatted string at the specified position
}

/// @brief  This self test exercises the LCD by performing a series of operations
/// that demonstrate basic functionality. It clears the display, moves the cursor
/// around, prints upper and lower case alphabets, symbols, and numbers.  It also
/// demonstrates some of the basic capabilities of the HD44780 controller by performing
/// display shifts, cursor movements, and other operations. It also
/// demonstrates the backlight control function by toggling the backlight on and
/// off during the test.
/// @param lcd Pointer to the lcd_handle_t structure that contains the configuration
/// for the LCD interface. This structure must be properly initialized with the 
/// correct interface mode, number of rows and columns, data port and control pin 
/// configurations, and a buffer for storing the current display contents. The 
/// function will use this information to send each character of the string to the 
/// LCD as data, and it will also update the display buffer accordingly. The 
/// function will continue to print characters until the end of the string is reached.
/// @param  None
/// @return None
void LCD_SelfTest(lcd_handle_t *lcd)
{
    LCD_Clear(lcd);
    LCD_PrintAt(lcd, 1, 1, "LCD Self Test");
    LCD_SetCursor(lcd, 1, 1);
    for (uint8_t i = 1; i <= 20; i++)
    {
        LCD_SetCursor(lcd, 1, i); // Move cursor to column i of the first line
        __delay_ms(500);
        LCD_SetCursor(lcd, 2, 1); // Display the current column number on the second line
        LCD_PrintfAt(lcd, 2, 1, "Col: %d", i);
    }

    LCD_Clear(lcd);
    LCD_PrintAt(lcd, 1, 1, "LCD Self Test");
    LCD_PrintAt(lcd, 2, 1, "abcdefghijklmnopqrst");
    LCD_PrintAt(lcd, 3, 1, "uvwxyz");
    __delay_ms(2000);
    LCD_PrintAt(lcd, 2, 1, "ABCDEFGHIJKLMNOPQRST");
    LCD_PrintAt(lcd, 3, 1, "UCWXYZ");
    __delay_ms(2000);
    LCD_PrintAt(lcd, 2, 1, "12345678901234567890");
    LCD_PrintAt(lcd, 3, 1, "!@#$%^&*()_+-=~`");
    __delay_ms(2000);
    LCD_Clear(lcd);
    LCD_PrintAt(lcd, 1, 1, "LCD Self Test");
    LCD_BackLight(lcd, false); // Turn off backlight
    __delay_ms(2000);
    LCD_BackLight(lcd, true); // Turn on backlight
    LCD_PrintAt(lcd, 2, 1, "Left");
    const char *centered = "Centered";
    const char *right = "Right";
    uint8_t centered_col = (uint8_t)(((20 - strlen(centered)) / 2) + 1);
    uint8_t right_col = (uint8_t)(20 - strlen(right) + 1);
    LCD_PrintAt(lcd, 3, centered_col, centered);
    LCD_PrintAt(lcd, 4, right_col, right);
    __delay_ms(2000);

    // Clear  the display and print a message at the end of line 4, then shift the
    // display left 10 characters, which will cause the message to scroll across
    // the display from right to left.
    LCD_Clear(lcd);
    LCD_PrintAt(lcd, 4, 14, "Scroll");
    for (int i = 0; i < 10; i++)
    {
        LCD_SendCommand(lcd, LCD_CMD_CURSOR_SHIFT | LCD_DISPLAY_MOVE | LCD_MOVE_LEFT);
        __delay_ms(500);
    }
    __delay_ms(2000);

    // Now shift the display back to the right to return it to the original position
    for (int i = 0; i < 10; i++)
    {
        LCD_SendCommand(lcd, LCD_CMD_CURSOR_SHIFT | LCD_DISPLAY_MOVE | LCD_MOVE_RIGHT);
        __delay_ms(500);
    }
    __delay_ms(2000);

    // Clear the display and now lets print every character in the LCD character
    // set by writing the character codes directly to the display as data bytes,
    // 80 bytes at a time, with a 1 second delay between each batch.  This will
    // demonstrate the full character set of the LCD, including the custom
    // characters that can be created in CGRAM, which are typically mapped to
    // character codes 0-7.
    LCD_Clear(lcd);
    uint8_t line = 1;
    int code = 0;
    while (code < 256)
    {
        for (line = 1; line <= 4; line++)
        {
            LCD_SetCursor(lcd, line, 1); // Move cursor to the beginning of the current line
            for (uint8_t col = 1; col <= 20 && code < 256; col++, code++)
            {
                LCD_SendData(lcd, (uint8_t)code & 0xFF); // Send the character code as a data byte to display it
            }
        }
        __delay_ms(1000); // Delay after every 20 characters (one line)
        LCD_Clear(lcd);
    }
}
