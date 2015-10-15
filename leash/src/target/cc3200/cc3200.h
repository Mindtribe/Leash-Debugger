/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

/*
 * The encompassing interface header for the CC3200 target.
 */

#ifndef CC3200_H_
#define CC3200_H_

#include "target_al.h"

//functions for the CC3200 are only visible to other modules
//through the generic target_al_interface in the form of
//function pointers.
extern struct target_al_interface cc3200_interface;

#endif
