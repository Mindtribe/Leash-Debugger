/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "jtag_pinctl.h"

#include <stdint.h>

#include "hw_types.h"
#include "gpio_if.h"
#include "utils.h"
#include "rom_map.h"
#include "gpio.h"

#include "jtag_statemachine.h"
#include "error.h"

#define JTAG_SET_PIN(REG, VAL, ONOFF) (HWREG((REG))=(ONOFF)*(VAL))
#define JTAG_GET_PIN(REG) (HWREG(REG))
#define TMS_REG 0x40006008
#define TMS_VAL 0x02
#define TDI_REG 0x40005200
#define TDI_VAL 0x80
#define TCK_REG 0x40007040
#define TCK_VAL 0x10
#define TDO_REG 0x40006100
#define TDO_VAL 0x40
#define RST_REG 0x40006003
#define RST_VAL 0x01

//state struct
struct jtag_pinctl_state_t{
    unsigned char initialized;
    unsigned char lastTDO;
};
//instantiated state structMAP_UtilsDelay
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
    .ucPin = 22,
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
const int TDO_SETUP_DELAY = 1;

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

    jtag_statemachine_reset();

    jtag_pinctl_state.initialized = 1;
    return RET_SUCCESS;
}

int jtag_pinctl_assertPins(uint8_t pins)
{
    if(!jtag_pinctl_state.initialized) RETURN_ERROR(ERROR_UNKNOWN); //not initialized

    if(pins & JTAG_RST) {
        GPIO_IF_Set(RSTLocation.ucPin,
                RSTLocation.uiGPIOPort,
                RSTLocation.ucGPIOPin, 0);
    }

    if(pins & JTAG_TMS) {
        GPIO_IF_Set(TMSLocation.ucPin,
                TMSLocation.uiGPIOPort,
                TMSLocation.ucGPIOPin, 1);
    }

    if(pins & JTAG_TDI){
        GPIO_IF_Set(TDILocation.ucPin,
                TDILocation.uiGPIOPort,
                TDILocation.ucGPIOPin, 1);
    }

    if(pins & JTAG_TCK){
        GPIO_IF_Set(TCKLocation.ucPin,
                TCKLocation.uiGPIOPort,
                TCKLocation.ucGPIOPin, 1);
    }

    return RET_SUCCESS;

}

int jtag_pinctl_deAssertPins(uint8_t pins)
{
    if(!jtag_pinctl_state.initialized) RETURN_ERROR(ERROR_UNKNOWN); //not initialized

    if(pins & JTAG_RST) {
        GPIO_IF_Set(RSTLocation.ucPin,
                RSTLocation.uiGPIOPort,
                RSTLocation.ucGPIOPin, 1);
    }

    if(pins & JTAG_TMS) {
        GPIO_IF_Set(TMSLocation.ucPin,
                TMSLocation.uiGPIOPort,
                TMSLocation.ucGPIOPin, 0);
    }

    if(pins & JTAG_TDI) {
        GPIO_IF_Set(TDILocation.ucPin,
                TDILocation.uiGPIOPort,
                TDILocation.ucGPIOPin, 0);
    }

    if(pins & JTAG_TCK) {
        GPIO_IF_Set(TCKLocation.ucPin,
                TCKLocation.uiGPIOPort,
                TCKLocation.ucGPIOPin, 0);
    }

    return RET_SUCCESS;
}

//asserts JTAG clock with the specified pins active.
int jtag_pinctl_doClock(uint8_t active_pins)
{
    if(!jtag_pinctl_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN);} //not initialized

    //determine which pins to set
    unsigned char TMS, TDI;
    TMS = (active_pins & JTAG_TMS) ? 1 : 0;
    TDI = (active_pins & JTAG_TDI) ? 1 : 0;

    jtag_statemachine_transition(TMS);
    JTAG_SET_PIN(TMS_REG, TMS_VAL, TMS);
    JTAG_SET_PIN(TDI_REG, TDI_VAL, TDI);

    JTAG_SET_PIN(TCK_REG, TCK_VAL, 1);

    MAP_UtilsDelay(TDO_SETUP_DELAY);
    jtag_pinctl_state.lastTDO = JTAG_GET_PIN(TDO_REG) ? 1:0;

    JTAG_SET_PIN(TCK_REG, TCK_VAL, 0);

    return RET_SUCCESS;
}

unsigned char jtag_pinctl_getLastTDO(void)
{
    return jtag_pinctl_state.lastTDO;
}

