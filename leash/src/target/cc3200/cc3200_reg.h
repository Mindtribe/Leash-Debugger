/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

/*
 * Contains the register struct (used as GDB packet format too) for the Cortex M4 found in the CC3200.
 */

#ifndef CC3200_REG_H_
#define CC3200_REG_H_

struct cc3200_reg_list{
    uint32_t r[12];
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
};

enum cc3200_reg_index{
    CC3200_REG_0 = 0,
    CC3200_REG_1,
    CC3200_REG_2,
    CC3200_REG_3,
    CC3200_REG_4,
    CC3200_REG_5,
    CC3200_REG_6,
    CC3200_REG_7,
    CC3200_REG_8,
    CC3200_REG_9,
    CC3200_REG_10,
    CC3200_REG_11,
    CC3200_REG_12,
    CC3200_REG_SP,
    CC3200_REG_LR,
    CC3200_REG_PC,
    CC3200_REG_XPSR,

    //for counting
    CC3200_REG_LAST
};

#endif
