/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "common.h"

uint32_t flip_endian(uint32_t input){
    return (input<<24)
            | ((input&0x0000FF00)<<8)
            | ((input&0x00FF0000)>>8)
            | ((input&0xFF000000)>>24);
}

void wfd_itoa(int num, char* string)
{
    int i = 0, temp_num = num, length = 0;
    int neg = 0;

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

void wfd_strncpy(char* dest, char* src, int max_size)
{
    for(int i=0; i<max_size; i++){
        if(i >= max_size) return;
        dest[i] = src[i];
        if(src[i] == 0) return;
    }
}
