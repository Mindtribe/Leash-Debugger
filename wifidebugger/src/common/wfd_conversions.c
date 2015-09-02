/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "wfd_conversions.h"

#include "wfd_string.h"

#include "crc32.h"

uint32_t wfd_flip_endian(uint32_t input){
    return (input<<24)
            | ((input&0x0000FF00)<<8)
            | ((input&0x00FF0000)>>8)
            | ((input&0xFF000000)>>24);
}

void wfd_itoa(int num, char* string)
{
    int i = 0, temp_num = num, length = 0;

    if(num < 0){
        num = -1 * num;
        string[0] = '-';
        string = &(string[1]);
    }
    else if(num==0){
        string[0] = '0';
        string[1] = 0;
        return;
    }

    while(temp_num) {
        temp_num /= 10;
        length++;
    }
    for(i = 0; i < length; i++) {
        string[(length-1)-i] = '0' + (num % 10);
        num /= 10;
    }
    string[i] = '\0';

    return;
}

void wfd_byteToHex(uint8_t byte, char* dst)
{
    char* hNibble = dst;
    char* lNibble = &(dst[1]);
    uint8_t i_lNibble, i_hNibble;
    i_lNibble = byte%16;
    i_hNibble = byte/16;

    if(i_lNibble < 10) *lNibble = '0' + (char)i_lNibble;
    else *lNibble = 'A' + ((char)i_lNibble-10);
    if(i_hNibble < 10) *hNibble = '0' + (char)i_hNibble;
    else *hNibble = 'A' + ((char)i_hNibble-10);
    return;
}

void wfd_wordToHex(uint32_t word, char* dst)
{
    wfd_byteToHex((uint8_t)(word>>24), &(dst[0]));
    wfd_byteToHex((uint8_t)(word>>16), &(dst[2]));
    wfd_byteToHex((uint8_t)(word>>8), &(dst[4]));
    wfd_byteToHex((uint8_t)(word>>0), &(dst[6]));
}

uint8_t wfd_hexToByte(char* src)
{

    uint8_t byte = 0;
    char hnibble, lnibble;
    hnibble = wfd_toUpperCaseHex(src[0]);
    lnibble = wfd_toUpperCaseHex(src[1]);

    if((hnibble >= '0') && (hnibble <= '9')) {byte += (hnibble - '0')*16; }
    else {byte += (hnibble - 'A' + 10)*16;}
    if((lnibble >= '0') && (lnibble <= '9')) {byte += (lnibble - '0'); }
    else {byte += (lnibble - 'A' + 10);}

    return byte;
}

int wfd_hexToInt(char* src)
{
    int result = 0;
    int len = wfd_strlen(src);
    int inv = 0;

    if(src[0] == '-'){
        inv = 1;
        src = &(src[1]);
        len--;
    }

    for(int i=0; i<len; i++){
        if((src[i] >= '0') && (src[i] <= '9')) {result += (src[i] - '0')<<(4*(len-i-1)); }
        else if((src[i] >= 'a') && (src[i] <= 'f')) {result += (src[i] - 'a' + 10)<<(4*(len-i-1)); }
        else {result += (src[i] - 'A' + 10)<<(4*(len-i-1)); }
    }
    return inv ? -result : result;
}

char wfd_toUpperCaseHex(char c)
{
    if((c>='a') && (c<='f')) return c-32;
    return c;
}

unsigned long long wfd_crc32(uint8_t *buf, uint32_t bufLen, unsigned long long crc)
{
    return crc32(buf, bufLen, crc);
}

int wfd_isHex(char c)
{
    if((c>='0') && (c<='9')) return 1;
    if((c>='A') && (c<='F')) return 1;
    if((c>='a') && (c<='f')) return 1;
    return 0;
}
