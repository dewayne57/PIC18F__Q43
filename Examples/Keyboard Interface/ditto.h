/* *****************************************************************************************
 *   File Name: ditto.h
 *   Description: This header defines the function(s) that produce a old-style "ditto" dump
 *   of a block of data.  This is loosely based on the old IBM mainframe Ditto utility 
 *   programs data dump feature. 
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
 ***************************************************************************************** */

 #ifndef DITTO_H
 #define DITTO_H

#include <stdint.h>
#include <stddef.h>

#define DITTO_BYTES_PER_LINE 48U

/// @brief Produces an old-style "ditto" dump of a block of data.
/// @param data  Pointer to the data block to dump.
/// @param length  Length of the data block in bytes.
void DittoDump(const uint8_t *data, size_t length);

#endif // DITTO_H
 