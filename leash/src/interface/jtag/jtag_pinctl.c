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

#include "error.h"

#define JTAG_SET_PIN(REG, VAL, ONOFF) (HWREG((REG))=((ONOFF)*(VAL)))
#define JTAG_GET_PIN(REG) (HWREG(REG))
#define TMS_REG 0x40006008
#define TMS_VAL 0x02
#define TDI_REG 0x40005200
#define TDI_VAL 0x80
#define TCK_REG 0x40007040
#define TCK_VAL 0x10
#define TDO_REG 0x40006100
#define TDO_VAL 0x40
#define RST_REG 0x40006004
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

struct jtagPinLocation RSTLocation = {
    .ucPin = 16,
    .uiGPIOPort = 0,
    .ucGPIOPin = 0
};

//"unit delay" (min TCK clock period becomes 4 times this)
const int TDO_SETUP_DELAY = 1;

//initializes JTAG pins to inactive.
int jtag_pinctl_init(void)
{
    if(jtag_pinctl_state.initialized) return RET_SUCCESS; //already initialized

    GPIO_IF_GetPortNPin(RSTLocation.ucPin,
            &(RSTLocation.uiGPIOPort),
            &(RSTLocation.ucGPIOPin));

    //default pin levels (inactive)
    JTAG_SET_PIN(TMS_REG, TMS_VAL, 0);
    JTAG_SET_PIN(TDI_REG, TDI_VAL, 0);
    JTAG_SET_PIN(RST_REG, RST_VAL, 1);
    JTAG_SET_PIN(TCK_REG, TCK_VAL, 0);

    jtag_pinctl_state.initialized = 1;
    return RET_SUCCESS;
}

int jtag_pinctl_assertPins(uint8_t pins)
{
    if(!jtag_pinctl_state.initialized) RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail"); //not initialized

    if(pins & JTAG_RST) {
        JTAG_SET_PIN(RST_REG, RST_VAL, 0);
    }

    if(pins & JTAG_TMS) {
        JTAG_SET_PIN(TMS_REG, TMS_VAL, 1);
    }

    if(pins & JTAG_TDI){
        JTAG_SET_PIN(TDI_REG, TDI_VAL, 1);
    }

    if(pins & JTAG_TCK){
        JTAG_SET_PIN(TCK_REG, TCK_VAL, 1);
    }

    return RET_SUCCESS;

}

int jtag_pinctl_deAssertPins(uint8_t pins)
{
    if(!jtag_pinctl_state.initialized) RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail"); //not initialized

    if(pins & JTAG_RST) {
        JTAG_SET_PIN(RST_REG, RST_VAL, 1);
    }

    if(pins & JTAG_TMS) {
        JTAG_SET_PIN(TMS_REG, TMS_VAL, 0);
    }

    if(pins & JTAG_TDI){
        JTAG_SET_PIN(TDI_REG, TDI_VAL, 0);
    }

    if(pins & JTAG_TCK){
        JTAG_SET_PIN(TCK_REG, TCK_VAL, 0);
    }

    return RET_SUCCESS;
}

void jtag_pinctl_doClock(uint8_t TMS, uint8_t TDI, uint8_t* TDO_result)
{
    JTAG_SET_PIN(TMS_REG, TMS_VAL, TMS);
    JTAG_SET_PIN(TDI_REG, TDI_VAL, TDI);
    JTAG_SET_PIN(TCK_REG, TCK_VAL, 1);

    MAP_UtilsDelay(TDO_SETUP_DELAY);
    *TDO_result = JTAG_GET_PIN(TDO_REG) ? 1:0;

    JTAG_SET_PIN(TCK_REG, TCK_VAL, 0);
    return;
}

