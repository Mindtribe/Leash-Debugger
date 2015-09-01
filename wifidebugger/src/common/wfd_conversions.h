/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#ifndef WFD_CONVERSIONS_H_
#define WFD_CONVERSIONS_H_

#include <stdint.h>

uint32_t wfd_flip_endian(uint32_t input);
void wfd_itoa(int num, char* string);
void wfd_byteToHex(uint8_t byte, char* dst);
uint8_t wfd_hexToByte(char* src);
char wfd_toUpperCaseHex(char c);
int wfd_hexToInt(char* src);
unsigned long long wfd_crc32(uint8_t *buf, uint32_t bufLen, unsigned long long crc);
void wfd_wordToHex(uint32_t word, char* dst);
int wfd_isHex(char c);

#endif
