/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include <stdint.h>

#include "cc3200_jtagdp.h"
#include "jtag_scan.h"
#include "common.h"

struct cc3200_jtagdp_state_t{
    uint8_t initialized;
    uint8_t detected;
    int num_precede_ir_bits;
    int num_precede_dr_bits;
    uint64_t precede_dr_bits;
    uint64_t precede_ir_bits;
    uint16_t DESIGNER;
    uint16_t PARTNO;
    uint8_t VERSION;
};

struct cc3200_jtagdp_state_t cc3200_jtagdp_state = {
    .initialized = 0,
    .detected = 0,
    .num_precede_ir_bits = 0,
    .precede_ir_bits = 0,
    .num_precede_dr_bits = 0,
    .precede_dr_bits = 0,
    .DESIGNER = 0,
    .PARTNO = 0,
    .VERSION = 0
};

//helper function prototypes
int cc3200_jtagdp_accResponseWrite(uint8_t* response, enum jtag_state_scan fromState, enum jtag_state_scan toState);
int cc3200_jtagdp_accResponseRead(uint8_t* response, uint32_t* value, enum jtag_state_scan fromState, enum jtag_state_scan toState);

int cc3200_jtagdp_init(int num_precede_ir_bits, uint64_t precede_ir_bits, int num_precede_dr_bits, uint64_t precede_dr_bits)
{
    cc3200_jtagdp_state.num_precede_ir_bits = num_precede_ir_bits;
    cc3200_jtagdp_state.precede_ir_bits = precede_ir_bits;
    cc3200_jtagdp_state.num_precede_dr_bits = num_precede_dr_bits;
    cc3200_jtagdp_state.precede_dr_bits = precede_dr_bits;
    cc3200_jtagdp_state.detected = 0;
    cc3200_jtagdp_state.initialized = 1;
    return RET_SUCCESS;
}

int cc3200_jtagdp_detect(void)
{
    if(!cc3200_jtagdp_state.initialized) return RET_FAILURE;

    //read the IDCODE of the core JTAG debug port.
    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_IDCODE,
            JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) return RET_FAILURE;
    if(cc3200_jtagdp_shiftDR(0, CC3200_JTAGDP_IDCODE_LEN,
            JTAG_STATE_SCAN_PAUSE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) return RET_FAILURE;
    uint32_t idcode = (uint32_t) (jtag_scan_getShiftOut() & 0xFFFFFFFF);

    //validate IDCODE
    cc3200_jtagdp_state.DESIGNER = (idcode>>1) & 0x7FF;
    cc3200_jtagdp_state.PARTNO = (idcode>>12) & 0xFFFF;
    cc3200_jtagdp_state.VERSION = (idcode>>28) & 0xF;
    if(cc3200_jtagdp_state.DESIGNER != CC3200_JTAGDP_DESIGNER_ARM) return RET_FAILURE;

    cc3200_jtagdp_state.detected = 1;
    return RET_SUCCESS;
}

int cc3200_jtagdp_shiftIR(uint8_t data, enum jtag_state_scan fromState, enum jtag_state_scan toState)
{
    return jtag_scan_shiftIR((cc3200_jtagdp_state.precede_ir_bits << CC3200_JTAGDP_IR_LEN) | (data&0xF),
            cc3200_jtagdp_state.num_precede_ir_bits + CC3200_JTAGDP_IR_LEN,
            fromState, toState);
}

int cc3200_jtagdp_shiftDR(uint64_t data, int DR_len, enum jtag_state_scan fromState, enum jtag_state_scan toState)
{
    return jtag_scan_shiftDR((cc3200_jtagdp_state.precede_dr_bits << DR_len) | data,
            cc3200_jtagdp_state.num_precede_dr_bits + DR_len,
            fromState, toState);
}

int cc3200_jtagdp_accResponseWrite(uint8_t* response, enum jtag_state_scan fromState, enum jtag_state_scan toState)
{
    //uint64_t shift_command = 1 | (0x0C >> 1);
    uint64_t shift_command = 1 | (0x04 >> 1); //read control/status register

    if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
            fromState, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) return RET_FAILURE;

    uint64_t result = jtag_scan_getShiftOut();
    *response = (uint8_t)(result & 0x07);

    if( (*response != CC3200_JTAGDP_WAIT) && (*response != CC3200_JTAGDP_OKFAULT) ) return RET_FAILURE; //invalid response

    //check control/status register
    shift_command = 1 | (0x0C >> 1); //dummy command

    if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
            JTAG_STATE_SCAN_PAUSE, toState) == RET_FAILURE) return RET_FAILURE;

    uint64_t result_status = jtag_scan_getShiftOut();

    if( ((uint8_t)(result_status & 0x07) != CC3200_JTAGDP_WAIT) &&
            ((uint8_t)(result_status & 0x07) != CC3200_JTAGDP_OKFAULT) ) return RET_FAILURE; //invalid response

    if((result_status >> 3) & (CC3200_JTAGDP_STICKYERR
            | CC3200_JTAGDP_WDATAERR
            | CC3200_JTAGDP_STICKYCMP)){
        while(1){};//return RET_FAILURE;
    }
    if(!((result_status >> 3) & CC3200_JTAGDP_READOK)){
        while(1){};//return RET_FAILURE;
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_accResponseRead(uint8_t* response, uint32_t* value, enum jtag_state_scan fromState, enum jtag_state_scan toState)
{
    //uint64_t shift_command = 1 | (0x0C >> 1);
    uint64_t shift_command = 1 | (0x04 >> 1); //read control/status register

    if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
            fromState, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) return RET_FAILURE;

    uint64_t result = jtag_scan_getShiftOut();
    *response = (uint8_t)(result & 0x07);
    *value = (uint32_t)(result >> 3);

    if( (*response != CC3200_JTAGDP_WAIT) && (*response != CC3200_JTAGDP_OKFAULT) ) return RET_FAILURE; //invalid response

    //check control/status register
    shift_command = 1 | (0x0C >> 1); //dummy command

    if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
            JTAG_STATE_SCAN_PAUSE, toState) == RET_FAILURE) return RET_FAILURE;

    uint64_t result_status = jtag_scan_getShiftOut();

    if( ((uint8_t)(result_status & 0x07) != CC3200_JTAGDP_WAIT) &&
            ((uint8_t)(result_status & 0x07) != CC3200_JTAGDP_OKFAULT) ) return RET_FAILURE; //invalid response

    if((result_status >> 3) & (CC3200_JTAGDP_STICKYERR
            | CC3200_JTAGDP_WDATAERR
            | CC3200_JTAGDP_STICKYCMP)){
        while(1){};//return RET_FAILURE;
    }
    if(!((result_status >> 3) & CC3200_JTAGDP_READOK)){
        while(1){};//return RET_FAILURE;
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_DPACC_write(uint8_t addr, uint32_t value)
{
    uint64_t shift_command = ((addr&0x0C) >> 1) | ((uint64_t) value) << 3;
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_DPACC,
            JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) return RET_FAILURE;

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
                JTAG_STATE_SCAN_PAUSE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) return RET_FAILURE;

        if(cc3200_jtagdp_accResponseWrite(&response,
                JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) return RET_FAILURE;
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_DPACC_read(uint8_t addr, uint32_t* result)
{
    uint64_t shift_command = 1 | ((addr&0x0C) >> 1);
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_DPACC,
            JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) return RET_FAILURE;

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
                JTAG_STATE_SCAN_PAUSE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) return RET_FAILURE;

        if(cc3200_jtagdp_accResponseRead(&response, result,
                JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) return RET_FAILURE;
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_APACC_write(uint8_t addr, uint32_t value)
{
    uint64_t shift_command = ((addr&0x0C) >> 1) | ((uint64_t) value) << 3;
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_APACC,
            JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) return RET_FAILURE;

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_APACC_LEN,
                JTAG_STATE_SCAN_PAUSE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) return RET_FAILURE;

        if(cc3200_jtagdp_accResponseWrite(&response,
                JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) return RET_FAILURE;
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_APACC_read(uint8_t addr, uint32_t* result)
{
    uint64_t shift_command = 1 | ((addr&0x0C) >> 1);
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_APACC,
            JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) return RET_FAILURE;

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_APACC_LEN,
                JTAG_STATE_SCAN_PAUSE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) return RET_FAILURE;

        if(cc3200_jtagdp_accResponseRead(&response, result,
                JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) return RET_FAILURE;
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_powerUpSystem(void)
{
    if(cc3200_jtagdp_DPACC_write(0x04, CC3200_JTAGDP_CSYSPWRUPREQ) == RET_FAILURE) return RET_FAILURE;

    uint32_t temp;
    for(int i=0; i<CC3200_JTAGDP_PWRUP_RETRIES; i++){
        if(cc3200_jtagdp_DPACC_read(0x04, &temp) == RET_FAILURE) return RET_FAILURE;
        if(temp & CC3200_JTAGDP_CSYSPWRUPACK) return RET_SUCCESS;
    }

    return RET_FAILURE;
}

int cc3200_jtagdp_powerUpDebug(void)
{
    if(cc3200_jtagdp_DPACC_write(0x04, CC3200_JTAGDP_CDBGPWRUPREQ) == RET_FAILURE) return RET_FAILURE;

    uint32_t temp;
    for(int i=0; i<CC3200_JTAGDP_PWRUP_RETRIES; i++){
        if(cc3200_jtagdp_DPACC_read(0x04, &temp) == RET_FAILURE) return RET_FAILURE;
        if(temp & CC3200_JTAGDP_CDBGPWRUPACK) return RET_SUCCESS;
    }

    return RET_FAILURE;
}
