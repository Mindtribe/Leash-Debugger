/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#ifndef WFD_STRING_H_
#define WFD_STRING_H_

#include <stdint.h>

uint32_t wfd_strlen(char* src);
int wfd_strncpy(char* dest, char* src, int max_size);
int wfd_stringsEqual(char* src1, char* src2);

#endif
