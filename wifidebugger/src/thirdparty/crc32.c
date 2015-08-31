/* Main code for remote server for GDB.
   Copyright (C) 1989-2015 Free Software Foundation, Inc.

   This file is part of GDB. (modified)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "crc32.h"

/* Table used by the crc32 function to calcuate the checksum.  */

static unsigned int crc32_table[256] =
{0, 0};

/* Compute 32 bit CRC from inferior memory.

   On success, return 32 bit CRC.
   On failure, return (unsigned long long) -1.  */

unsigned long long crc32 (uint8_t* data, int len, unsigned int crc)
{
    uint8_t *bytes = data;
    if (!crc32_table[1]){
        /* Initialize the CRC table and the decoding table.  */
        int i, j;
        unsigned int c;

        for (i = 0; i < 256; i++){
            for (c = i << 24, j = 8; j > 0; --j)
                c = c & 0x80000000 ? (c << 1) ^ 0x04c11db7 : (c << 1);
            crc32_table[i] = c;
        }
    }

    while (len--){
        crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ bytes[0]) & 255];
        bytes = &(bytes[1]);
    }
    return (unsigned long long) crc;
}

