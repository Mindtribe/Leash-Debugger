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
