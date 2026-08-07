/* *****************************************************************************************
 *   File Name: ditto.c
 *   Description: Implementation of the Ditto module for producing old-style "ditto" dumps 
 *   of data blocks.  This is loosely based on the old IBM mainframe Ditto utility 
 *   programs data dump feature. 
 * 
 *   Author: Dewayne Hafenstein
 *   Date: 2026-05-19
 * 
 *   Ditto printed both hex and character values of each byte of the data block it dumped
 *   in a vertical stacked format.  The high nibble was on top, then the low nibble, then 
 *   the character equivalent (if any) or a "." if not printable. Every 8-bytes of data 
 *   was separated by one space to break the display into 8-byte chunks.  The start of each 
 *   line printed the hexadecimal offset of the first byte in that line.
 * 
 *   For example, if the data block contained the bytes 0x41, 0x42, 0x43, 0x44, the Ditto dump 
 *   would display:
 * 
 *   Dump of data block at 0xDEADBEEF for a length of 4 bytes:
 *   00000000: 4444 
 *             1234
 *             ABCD
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
#include <stdio.h>
#include <string.h>
#include "ditto.h"

#define CRLF "\r\n"

/// @brief Prints the high nibble of a byte in blocks of 8, separated by a space, 
/// and ended by a CRLF
/// @param byte  The byte whose high nibble is to be printed.
/// @param width  The width of the block in which the high nibble is printed.
static void printHighNybble(uint8_t *byte, int width) {
    for (int i = 0; i < width; i++) {
        printf("%X", (int) ((*byte >> 4) & 0x0F));
        if ((i + 1) % 8 == 0) {
            printf(" ");
        }
    }
    printf("%s", CRLF);
}

/// @brief Prints the low nibble of a byte in blocks of 8, separated by a space, 
/// and ended by a CRLF
/// @param byte  The byte whose low nibble is to be printed.
/// @param width  The width of the block in which the low nibble is printed.
static void printLowNybble(uint8_t *byte, int width) {
    printf("          ");   // Adjust for the offset value. 
    for (int i = 0; i < width; i++) {
        printf("%X", (int) (*byte & 0x0F));
        if ((i + 1) % 8 == 0) {
            printf(" ");
        }
    }
    printf("%s", CRLF);
}

static void printCharacters(uint8_t *byte, int width) {
    printf("          ");   // Adjust for the offset value. 
    for (int i = 0; i < width; i++) {
        uint8_t c = *byte & 0xFF;
        if (c >= 32 && c <= 126) {
            printf("%c", c);
        } else {
            printf(".");
        }
        if ((i + 1) % 8 == 0) {
            printf(" ");
        }
    }
    printf("%s%s", CRLF, CRLF);
}

/// @brief Produces an old-style "ditto" dump of a block of data.
/// @param data  Pointer to the data block to dump.
/// @param length  Length of the data block in bytes.
void DittoDump(const uint8_t *data, size_t length) {

    // The user must supply something to dump.
    if (length == 0 || data == (void *)0) {
        return; 
    }

    // A line will contain at most DITTO_BYTES_PER_LINE bytes of data, displayed as 8 groups of 
    // 8 bytes, with a 4-byte offset on the first line of each dump output. 
    int lineCount = length / DITTO_BYTES_PER_LINE;
    int remainder = length % DITTO_BYTES_PER_LINE;
    int offset = 0; 
    uint8_t *byteData;

    printf("Dump of data block at %p for a length of %zu bytes:%s", data, length, CRLF);
    for (int i = 0; i < lineCount; i++) {
        byteData = (uint8_t *)data + offset;

        printf("%08X: ", offset);
        printHighNybble(byteData, DITTO_BYTES_PER_LINE);
        printLowNybble(byteData, DITTO_BYTES_PER_LINE);
        printCharacters(byteData, DITTO_BYTES_PER_LINE);

        offset += DITTO_BYTES_PER_LINE;
    }

    // Print the remaining bytes, if any
    if (remainder > 0) {
        byteData = (uint8_t *)data + offset;
        printf("%08X: ", offset);
        printHighNybble(byteData, remainder);
        printLowNybble(byteData, remainder);
        printCharacters(byteData, remainder);

        printf("%s", CRLF);
    }
}