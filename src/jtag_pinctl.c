/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include <stdint.h>

#include "hw_types.h"
#include "gpio_if.h"
#include "utils.h"
#include "rom_map.h"
#include "gpio.h"
#include "utils.h"

#include "jtag_pinctl.h"
#include "common.h"

//state struct
struct jtag_pinctl_state_t{
    unsigned char initialized;
    unsigned char lastTDO;
};
//instatiated state struct
static struct jtag_pinctl_state_t jtag_pinctl_state = {
    .initialized = 0,
    .lastTDO = 0
};

//GPIO pin assignments for JTAG lines
struct jtagPinLocation TDILocation = {
    .ucPin = 15,
    .uiGPIOPort = 0,
    .ucGPIOPin = 0
};
struct jtagPinLocation TDOLocation = {
    .ucPin = 1,
    .uiGPIOPort = 0,
    .ucGPIOPin = 0
};
struct jtagPinLocation TMSLocation = {
    .ucPin = 17,
    .uiGPIOPort = 0,
    .ucGPIOPin = 0
};
struct jtagPinLocation RSTLocation = {
    .ucPin = 16,
    .uiGPIOPort = 0,
    .ucGPIOPin = 0
};
struct jtagPinLocation TCKLocation = {
    .ucPin = 28,
    .uiGPIOPort = 0,
    .ucGPIOPin = 0
};

//"unit delay" (min TCK clock period becomes 4 times this)
const int UNIT_DELAY = 4000;

//initializes JTAG pins to inactive.
int jtag_pinctl_init(void)
{
    if(jtag_pinctl_state.initialized) return RET_SUCCESS; //already initialized

    //get pin details

    GPIO_IF_GetPortNPin(TDILocation.ucPin,
            &(TDILocation.uiGPIOPort),
            &(TDILocation.ucGPIOPin));

    GPIO_IF_GetPortNPin(TDOLocation.ucPin,
            &(TDOLocation.uiGPIOPort),
            &(TDOLocation.ucGPIOPin));

    GPIO_IF_GetPortNPin(RSTLocation.ucPin,
            &(RSTLocation.uiGPIOPort),
            &(RSTLocation.ucGPIOPin));

    GPIO_IF_GetPortNPin(TMSLocation.ucPin,
            &(TMSLocation.uiGPIOPort),
            &(TMSLocation.ucGPIOPin));

    GPIO_IF_GetPortNPin(TCKLocation.ucPin,
            &(TCKLocation.uiGPIOPort),
            &(TCKLocation.ucGPIOPin));

    //default pin levels (inactive)

    GPIO_IF_Set(TMSLocation.ucPin,
            TMSLocation.uiGPIOPort,
            TMSLocation.ucGPIOPin,
            0);

    GPIO_IF_Set(RSTLocation.ucPin,
            RSTLocation.uiGPIOPort,
            RSTLocation.ucGPIOPin,
            1);

    GPIO_IF_Set(TDILocation.ucPin,
            TDILocation.uiGPIOPort,
            TDILocation.ucGPIOPin,
            0);

    //clock HIGH
    GPIO_IF_Set(TCKLocation.ucPin,
            TCKLocation.uiGPIOPort,
            TCKLocation.ucGPIOPin,
            0);

    jtag_pinctl_state.initialized = 1;
    return RET_SUCCESS;
}

//asserts JTAG clock with the specified pins active.
int jtag_pinctl_doClock(uint8_t active_pins)
{
    if(!jtag_pinctl_state.initialized) return RET_FAILURE; //not initialized

    //determine which pins to set
    unsigned char TMS, TDI, RST;
    TMS = (active_pins & JTAG_TMS) ? 1 : 0;
    TDI = (active_pins & JTAG_TDI) ? 1 : 0;
    RST = (active_pins & JTAG_RST) ? 0 : 1; //inverted: active low

    GPIO_IF_Set(TMSLocation.ucPin,
            TMSLocation.uiGPIOPort,
            TMSLocation.ucGPIOPin,
            TMS);

    GPIO_IF_Set(RSTLocation.ucPin,
            RSTLocation.uiGPIOPort,
            RSTLocation.ucGPIOPin,
            RST);

    GPIO_IF_Set(TDILocation.ucPin,
            TDILocation.uiGPIOPort,
            TDILocation.ucGPIOPin,
            TDI);

    //TODO: delay check
    MAP_UtilsDelay(UNIT_DELAY);

    //clock HIGH
    GPIO_IF_Set(TCKLocation.ucPin,
            TCKLocation.uiGPIOPort,
            TCKLocation.ucGPIOPin,
            1);

    //TODO: delay check
    MAP_UtilsDelay(2*UNIT_DELAY);

    //read TDO just before falling edge and store
    jtag_pinctl_state.lastTDO = GPIO_IF_Get(TDOLocation.ucPin,
            TDOLocation.uiGPIOPort,
            TDOLocation.ucGPIOPin);

    //clock LOW
    GPIO_IF_Set(TCKLocation.ucPin,
            TCKLocation.uiGPIOPort,
            TCKLocation.ucGPIOPin,
            0);

    //TODO: read TDO

    //TODO: delay check
    MAP_UtilsDelay(UNIT_DELAY);

    return RET_SUCCESS;
}

unsigned char jtag_pinctl_getLastTDO(void)
{
    return jtag_pinctl_state.lastTDO;
}


