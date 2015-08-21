/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

#define RET_SUCCESS (0)
#define RET_FAILURE (-1)

#define MIN(X, Y) ((X > Y) ? Y : X)

uint32_t flip_endian(uint32_t input);
void wfd_itoa(int num, char* string);
int wfd_strncpy(char* dest, char* src, int max_size);
int wfd_stringsEqual(char* src1, char* src2);
void wfd_byteToHex(uint8_t byte, char* dst);
uint8_t wfd_hexToByte(char* src);
char wfd_toUpperCaseHex(char c);
uint32_t wfd_strlen(char* src);
uint32_t wfd_hexToInt(char* src);
unsigned long long wfd_crc32(uint8_t *buf, uint32_t bufLen, unsigned long long crc);

#endif
