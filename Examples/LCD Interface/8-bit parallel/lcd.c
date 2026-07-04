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
#include "config.h"
#include "lcd.h"

/// @brief This function reads the busy flag and returns true if the LCD is busy, 
/// or false if it is ready to accept the next command or data byte. This function
/// must be called before every command or data write to ensure the LCD is ready, and
/// it must also be called after initialization before the first command or data write.
/// In order to read the busy flag, the LCD data pins are temporarily reconfigured as inputs,
/// and the RW pin is set to read mode. After reading the busy flag, the data pins are
/// reconfigured as outputs and the RW pin is set back to write mode.
/// @param  None
/// @return true if the LCD is busy, false if it is ready for the next command or data byte
static bool LCD_ReadBusyFlag(void) {
    LCD_DATA_TRIS = 0xFF; // Configure data pins as inputs
    LCD_RW = 1; // Set RW to read mode
    LCD_E = 1; // Set E high to latch the busy flag on the data pins
    __delay_us(1); // Short delay to allow data to stabilize

    bool busy = LCD_DATA_PORT & 0x80; // Read the busy flag (D7)
    LCD_E = 0; // Set E low to complete the read cycle
    LCD_RW = 0; // Set RW back to write mode
    LCD_DATA_TRIS = 0x00; // Configure data pins as outputs
    return busy;
}

/// @brief  Initialize the LCD display. This function must be called before any other LCD functions are used.
/// @param  None
/// @return None
void LCD_Init(void) {
    // Wait for LCD to power up and become ready (busy flag will be high)
    while (LCD_ReadBusyFlag());

    // Function set: 8-bit mode, 2 lines, 5x8 dots
    LCD_SendCommand(LCD_CMD_FUNCTION_SET | LCD_8_BIT_MODE | LCD_2_LINE | LCD_5x10_DOTS);
    while (LCD_ReadBusyFlag());

    // Display control: display on, cursor off, blink off
    LCD_SendCommand(LCD_CMD_DISPLAY_CONTROL | LCD_DISPLAY_ON);
    while (LCD_ReadBusyFlag());

    // Clear display
    LCD_Clear();
    
    // Entry mode set: increment cursor, no display shift
    LCD_SendCommand(LCD_CMD_ENTRY_MODE_SET | LCD_ENTRY_MODE_INCREMENT);
    while (LCD_ReadBusyFlag());
}

/// @brief  Send a command to the LCD. 
/// @param  cmd  The command byte to send to the LCD
/// @return None
void LCD_SendCommand(uint8_t cmd) {
    while (LCD_ReadBusyFlag()); // Wait until LCD is not busy
    LCD_RS = 0; // Set RS low for command mode
    LCD_RW = 0; // Set RW low for write mode
    LCD_DATA_PORT = cmd; // Put the command byte on the data pins
    LCD_E = 1; // Set E high to latch the command
    __delay_us(1); // Short delay to allow data to be latched
    LCD_E = 0; // Set E low to complete the write cycle
}

/// @brief  Send a data byte to the LCD. 
/// @param  data  The data byte to send to the LCD
/// @return None
void LCD_SendData(uint8_t data) {
    while (LCD_ReadBusyFlag()); // Wait until LCD is not busy
    LCD_RS = 1; // Set RS high for data mode
    LCD_RW = 0; // Set RW low for write mode
    LCD_DATA_PORT = data; // Put the data byte on the data pins
    LCD_E = 1; // Set E high to latch the data
    __delay_us(1); // Short delay to allow data to be latched
    LCD_E = 0; // Set E low to complete the write cycle
}

/// @brief  Clear the LCD display and return the cursor to the home 
/// position (0,0).
/// @param  None
/// @return None
void LCD_Clear(void) {
    LCD_SendCommand(LCD_CMD_CLEAR_DISPLAY);
    __delay_ms(2); // Clear command takes longer time to execute
}

/// @brief  Set the cursor position on the LCD.
/// @param  row  The row number (0 or 1)
/// @param  col  The column number (0 to 15)
/// @return None
void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? col : (0x40 + col);
    LCD_SendCommand(LCD_CMD_SET_DDRAM_ADDR | address);
}

/// @brief  Print a string on the LCD. The string will be printed 
/// starting at the current cursor position. The cursor will 
/// automatically move to the right after each character is printed.
/// @param str  The string to print
/// @return None    
void LCD_Print(const char *str) {
    while (*str) {
        LCD_SendData((uint8_t)(*str)); // Send each character as data
        str++; // Move to the next character in the string
    }
}

