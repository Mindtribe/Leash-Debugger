/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */


#include "wfd_string.h"
#include "error.h"

uint32_t wfd_strlen(char* src)
{
    uint32_t i;
    for(i=0; src[i] != 0; i++);
    return i;
}

int wfd_strncpy(char* dest, char* src, int max_size)
{
    for(int i=0; i<max_size; i++){
        if(i >= max_size) return RET_FAILURE;
        dest[i] = src[i];
        if(src[i] == 0) return i;
    }

    return RET_FAILURE;
}

int wfd_stringsEqual(char* src1, char* src2){
    int i;
    for(i=0; src1[i] != 0 && src2[i] != 0; i++){
        if(src1[i] != src2[i]) return 0;
    }
    if(src1[i] == 0 && src2[i] == 0) return 1;
    return 0;
}
