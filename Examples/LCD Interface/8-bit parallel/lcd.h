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

void LCD_BackLight(bool state);
void LCD_Init(void);
void LCD_SendCommand(uint8_t cmd);
void LCD_SendData(uint8_t data);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(const char* str);

#endif /* LCD_H */