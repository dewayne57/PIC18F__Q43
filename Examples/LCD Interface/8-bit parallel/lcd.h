/* *******************************************************************************
 *   File Name: lcd.h
 *   Description: Header file for 8-bit LCD interface functions.
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
#ifndef LCD_H
#define LCD_H

#define LCD_DATA_PORT PORTD
#define LCD_DATA_TRIS TRISD
#define LCD_BUSY_FLAG PORTDbits.RD7
#define LCD_RS LATEbits.LATE0
#define LCD_RW LATEbits.LATE1
#define LCD_E LATEbits.LATE2
#define LCD_RS_TRIS TRISEbits.TRISE0
#define LCD_RW_TRIS TRISEbits.TRISE1
#define LCD_E_TRIS TRISEbits.TRISE2

// Define the DDRAM addresses for the start of each line on a 20x4 LCD
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
void LCD_SendCommand(uint8_t cmd);
void LCD_SendData(uint8_t data);
bool LCD_ReadBusyFlag(void);

// The following are the main functions for controlling the LCD.  These functions
// provide a convenient interface for initializing the LCD, clearing the display,
// setting the cursor position, and printing text.  The LCD_SelfTest function is a
// demonstration function that shows how to use the other functions to perform a
// series of tests on the LCD to verify that it is working correctly.
void LCD_SelfTest(void); 
void LCD_BackLight(bool state);
void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_ClearLine(uint8_t line);
void LCD_Print(const char* str);
void LCD_PrintAt(uint8_t row, uint8_t col, const char* str);
void LCD_Printf(const char* format, ...);
void LCD_PrintfAt(uint8_t row, uint8_t col, const char* format, ...);

#endif /* LCD_H */