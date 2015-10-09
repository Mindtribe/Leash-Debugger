/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

/*
 * General definition of breakpoints.
 */

enum breakpoint_type{
    BKPT_SOFTWARE = 0,
    BKPT_HARDWARE
};

struct breakpoint{
    uint32_t addr;
    uint8_t len_bytes;
    uint32_t ori_instr;
    uint8_t active;
    uint8_t valid;
    enum breakpoint_type type;
};
