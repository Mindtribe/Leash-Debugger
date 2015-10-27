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
#include "utils.h"
#include "rom_map.h"

#include "error.h"
#include "ui.h"
#include "gpio_al.h"

#ifdef PINOUT_LAUNCHPAD
#define GPIO_TMS 17
#define GPIO_TDI 15
#define GPIO_TCK 28
#define GPIO_TDO 22
#define GPIO_RST 16
#endif

#ifdef PINOUT_RBL_WIFIMINI
#define GPIO_TMS 15
#define GPIO_TDI 16
#define GPIO_TCK 14
#define GPIO_TDO 12
#define GPIO_RST 0
#endif

//result of most recent scan - can be accessed
//by other modules
uint64_t jtag_pinctl_scanout = 0;

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
struct jtagPinLocation{
    unsigned char ucPin;
    unsigned int uiGPIOPort;
    unsigned char ucGPIOPin;
};
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

    //default pin levels (inactive)
    GPIO_SET_PIN(GPIO_TMS, 0);
    GPIO_SET_PIN(GPIO_TDI, 0);
    GPIO_SET_PIN(GPIO_RST, 1);
    GPIO_SET_PIN(GPIO_TCK, 0);

    jtag_pinctl_state.initialized = 1;
    return RET_SUCCESS;
}

int jtag_pinctl_assertPins(uint8_t pins)
{
    if(!jtag_pinctl_state.initialized) RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail"); //not initialized

    if(pins & JTAG_RST) {
        GPIO_SET_PIN(GPIO_RST, 0);
    }

    if(pins & JTAG_TMS) {
        GPIO_SET_PIN(GPIO_TMS, 1);
    }

    if(pins & JTAG_TDI){
        GPIO_SET_PIN(GPIO_TDI, 1);
    }

    if(pins & JTAG_TCK){
        GPIO_SET_PIN(GPIO_TCK, 1);
    }

    return RET_SUCCESS;

}

int jtag_pinctl_deAssertPins(uint8_t pins)
{
    if(!jtag_pinctl_state.initialized) RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail"); //not initialized

    if(pins & JTAG_RST) {
        GPIO_SET_PIN(GPIO_RST, 1);
    }

    if(pins & JTAG_TMS) {
        GPIO_SET_PIN(GPIO_TMS, 0);
    }

    if(pins & JTAG_TDI){
        GPIO_SET_PIN(GPIO_TDI, 0);
    }

    if(pins & JTAG_TCK){
        GPIO_SET_PIN(GPIO_TCK, 0);
    }

    return RET_SUCCESS;
}

void jtag_pinctl_doClock(uint8_t TMS, uint8_t TDI, uint8_t* TDO_result)
{
    GPIO_SET_PIN(GPIO_TMS, TMS);
    GPIO_SET_PIN(GPIO_TDI, TDI);
    GPIO_SET_PIN(GPIO_TCK, 1);

    asm("nop");
    *TDO_result = GPIO_GET_PIN(GPIO_TDO) ? 1:0;

    GPIO_SET_PIN(GPIO_TCK, 0);
    return;
}

void jtag_pinctl_doStateMachine(uint32_t tms_bits_lsb_first, unsigned int num_clk)
{
    /*
    for(unsigned int i=0; i<num_clk; i++){
        uint8_t dummy;
        jtag_pinctl_doClock((tms_bits_lsb_first&(1<<i)) ? 1:0, 0, &dummy);
    }*/

    while(num_clk-- > 0){
        GPIO_SET_PIN(GPIO_TMS, (uint32_t)(tms_bits_lsb_first&1));
        GPIO_SET_PIN(GPIO_TCK, 1);
        asm("nop");
        GPIO_SET_PIN(GPIO_TCK, 0);
        tms_bits_lsb_first >>= 1;
    }
}

void jtag_pinctl_doData(uint64_t tdi_bits_lsb_first, unsigned int num_clk)
{

    jtag_pinctl_scanout = 0;

    for(unsigned int i=0; i<(num_clk-1); i++){
        GPIO_SET_PIN(GPIO_TDI, (tdi_bits_lsb_first & 1));
        GPIO_SET_PIN(GPIO_TCK, 1);
        asm("nop");
        if(GPIO_GET_PIN(GPIO_TDO))  jtag_pinctl_scanout |= (((uint64_t)1)<<((uint64_t)i));
        GPIO_SET_PIN(GPIO_TCK, 0);
        tdi_bits_lsb_first >>= 1;
    }
    //last bit with TMS high
    GPIO_SET_PIN(GPIO_TDI, (tdi_bits_lsb_first & 1));
    GPIO_SET_PIN(GPIO_TMS, 1);
    GPIO_SET_PIN(GPIO_TCK, 1);
    asm("nop");
    GPIO_SET_PIN(GPIO_TCK, 0);
}
