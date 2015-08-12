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
#include "error.h"
#include "mem_log.h"

//this struct represents an AP connected to the JTAG-DP.
struct cc3200_jtagdp_ap{
    uint32_t idcode;
};

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
    struct cc3200_jtagdp_ap ap[16];
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
    if(!cc3200_jtagdp_state.initialized) RETURN_ERROR(ERROR_UNKNOWN);

    //read the IDCODE of the core JTAG debug port.
    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_IDCODE,
            JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_jtagdp_shiftDR(0, CC3200_JTAGDP_IDCODE_LEN,
            JTAG_STATE_SCAN_PAUSE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    uint32_t idcode = (uint32_t) (jtag_scan_getShiftOut() & 0xFFFFFFFF);

    //validate IDCODE
    cc3200_jtagdp_state.DESIGNER = (idcode>>1) & 0x7FF;
    cc3200_jtagdp_state.PARTNO = (idcode>>12) & 0xFFFF;
    cc3200_jtagdp_state.VERSION = (idcode>>28) & 0xF;
    if(cc3200_jtagdp_state.DESIGNER != CC3200_JTAGDP_DESIGNER_ARM) RETURN_ERROR(ERROR_UNKNOWN);

    cc3200_jtagdp_state.detected = 1;
    return RET_SUCCESS;
}

int cc3200_jtagdp_selectAPBank(uint8_t AP, uint8_t bank)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    uint32_t APSELECT = (bank<<4) | AP<<24;
    if(cc3200_jtagdp_DPACC_write(0x08,APSELECT) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    return RET_SUCCESS;
}

int cc3200_jtagdp_readAPs(void)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    for(uint32_t i=0; i<16; i++){
        if(cc3200_jtagdp_selectAPBank(i, CC3200_JTAGDP_AP_IDCODE_BANK) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
        if(cc3200_jtagdp_APACC_read(CC3200_JTAGDP_AP_IDCODE_REG,
                &cc3200_jtagdp_state.ap[i].idcode) == RET_FAILURE) WAIT_ERROR(ERROR_UNKNOWN);
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_shiftIR(uint8_t data, enum jtag_state_scan fromState, enum jtag_state_scan toState)
{
    if(!cc3200_jtagdp_state.initialized) RETURN_ERROR(ERROR_UNKNOWN);

    return jtag_scan_shiftIR((cc3200_jtagdp_state.precede_ir_bits << CC3200_JTAGDP_IR_LEN) | (data&0xF),
            cc3200_jtagdp_state.num_precede_ir_bits + CC3200_JTAGDP_IR_LEN,
            fromState, toState);
}

int cc3200_jtagdp_shiftDR(uint64_t data, int DR_len, enum jtag_state_scan fromState, enum jtag_state_scan toState)
{
    if(!cc3200_jtagdp_state.initialized) RETURN_ERROR(ERROR_UNKNOWN);

    return jtag_scan_shiftDR((cc3200_jtagdp_state.precede_dr_bits << DR_len) | data,
            cc3200_jtagdp_state.num_precede_dr_bits + DR_len,
            fromState, toState);
}

int cc3200_jtagdp_accResponseWrite(uint8_t* response, enum jtag_state_scan fromState, enum jtag_state_scan toState)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    //uint64_t shift_command = 1 | (0x0C >> 1);
    uint64_t shift_command = 1 | (0x04 >> 1); //read control/status register

    if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
            fromState, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    uint64_t result = jtag_scan_getShiftOut();
    *response = (uint8_t)(result & 0x07);

    if( (*response != CC3200_JTAGDP_WAIT) && (*response != CC3200_JTAGDP_OKFAULT) ) RETURN_ERROR(ERROR_UNKNOWN); //invalid response

    //check control/status register
    shift_command = 1 | (0x0C >> 1); //dummy command

    if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
            JTAG_STATE_SCAN_PAUSE, toState) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    uint64_t result_status = jtag_scan_getShiftOut();

    if( ((uint8_t)(result_status & 0x07) != CC3200_JTAGDP_WAIT) &&
            ((uint8_t)(result_status & 0x07) != CC3200_JTAGDP_OKFAULT) ) RETURN_ERROR(ERROR_UNKNOWN); //invalid response

    if((result_status >> 3) & (CC3200_JTAGDP_STICKYERR
            | CC3200_JTAGDP_WDATAERR
            | CC3200_JTAGDP_STICKYCMP)){
        RETURN_ERROR(result_status>>3);
    }
    //if(!((result_status >> 3) & CC3200_JTAGDP_READOK)){
    //RETURN_ERROR(ERROR_UNKNOWN);
    //}

    return RET_SUCCESS;
}

int cc3200_jtagdp_accResponseRead(uint8_t* response, uint32_t* value, enum jtag_state_scan fromState, enum jtag_state_scan toState)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    //uint64_t shift_command = 1 | (0x0C >> 1);
    uint64_t shift_command = 1 | (0x04 >> 1); //read control/status register

    if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
            fromState, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    uint64_t result = jtag_scan_getShiftOut();
    *response = (uint8_t)(result & 0x07);
    *value = (uint32_t)(result >> 3);

    if( (*response != CC3200_JTAGDP_WAIT) && (*response != CC3200_JTAGDP_OKFAULT) ) RETURN_ERROR(ERROR_UNKNOWN); //invalid response

    //check control/status register
    shift_command = 1 | (0x0C >> 1); //dummy command

    if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
            JTAG_STATE_SCAN_PAUSE, toState) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    uint64_t result_status = jtag_scan_getShiftOut();

    if( ((uint8_t)(result_status & 0x07) != CC3200_JTAGDP_WAIT) &&
            ((uint8_t)(result_status & 0x07) != CC3200_JTAGDP_OKFAULT) ) RETURN_ERROR(ERROR_UNKNOWN); //invalid response

    if((result_status >> 3) & (CC3200_JTAGDP_STICKYERR
            | CC3200_JTAGDP_WDATAERR
            | CC3200_JTAGDP_STICKYCMP)){
        RETURN_ERROR(result_status>>3);
    }
    //if(!((result_status >> 3) & CC3200_JTAGDP_READOK)){
    //RETURN_ERROR(ERROR_UNKNOWN);
    //}

    return RET_SUCCESS;
}

int cc3200_jtagdp_DPACC_write(uint8_t addr, uint32_t value)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    uint64_t shift_command = ((addr&0x0C) >> 1) | ((uint64_t) value) << 3;
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_DPACC,
            JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
                JTAG_STATE_SCAN_PAUSE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

        if(cc3200_jtagdp_accResponseWrite(&response,
                JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_DPACC_read(uint8_t addr, uint32_t* result)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    uint64_t shift_command = 1 | ((addr&0x0C) >> 1);
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_DPACC,
            JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
                JTAG_STATE_SCAN_PAUSE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

        if(cc3200_jtagdp_accResponseRead(&response, result,
                JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_APACC_write(uint8_t addr, uint32_t value)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    uint64_t shift_command = ((addr&0x0C) >> 1) | ((uint64_t) value) << 3;
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_APACC,
            JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_APACC_LEN,
                JTAG_STATE_SCAN_PAUSE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

        if(cc3200_jtagdp_accResponseWrite(&response,
                JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_APACC_read(uint8_t addr, uint32_t* result)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    uint64_t shift_command = 1 | ((addr&0x0C) >> 1);
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_APACC,
            JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_APACC_LEN,
                JTAG_STATE_SCAN_PAUSE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

        if(cc3200_jtagdp_accResponseRead(&response, result,
                JTAG_STATE_SCAN_RUNIDLE, JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_powerUpDebug(void)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) RETURN_ERROR(ERROR_UNKNOWN);

    if(cc3200_jtagdp_DPACC_write(0x04, CC3200_JTAGDP_CDBGPWRUPREQ | CC3200_JTAGDP_CSYSPWRUPREQ) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    uint32_t temp;
    for(int i=0; i<CC3200_JTAGDP_PWRUP_RETRIES; i++){
        if(cc3200_jtagdp_DPACC_read(0x04, &temp) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
        if((temp & CC3200_JTAGDP_CDBGPWRUPACK) && (temp & CC3200_JTAGDP_CSYSPWRUPACK)) return RET_SUCCESS;
        mem_log_add("CSR value waiting for dbg powerup:", (int)temp);
    }

    RETURN_ERROR(ERROR_UNKNOWN);

    return RET_FAILURE;
}

int cc3200_jtagdp_clearCSR(void)
{
    if(cc3200_jtagdp_DPACC_write(0x04, CC3200_JTAGDP_STICKYERR
            | CC3200_JTAGDP_STICKYCMP
            | CC3200_JTAGDP_STICKYORUN) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    return RET_SUCCESS;
}

int cc3200_jtagdp_checkCSR(uint32_t* csr)
{
    if(cc3200_jtagdp_DPACC_read(0x04, csr) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    return RET_SUCCESS;
}
