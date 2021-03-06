/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */


#include "cc3200_jtagdp.h"

#include <stdint.h>

#include "log.h"
#include "jtag_scan.h"
#include "error.h"

//instruction register
#define CC3200_JTAGDP_IR_ABORT 0x08
#define CC3200_JTAGDP_IR_DPACC 0x0A
#define CC3200_JTAGDP_IR_APACC 0x0B
#define CC3200_JTAGDP_IR_IDCODE 0x0E
#define CC3200_JTAGDP_IR_BYPASS 0x0F
#define CC3200_JTAGDP_IR_LEN 4

//data register
#define CC3200_JTAGDP_IDCODE_LEN 32
#define CC3200_JTAGDP_DPACC_LEN 35
#define CC3200_JTAGDP_APACC_LEN 35

//properties
#define CC3200_JTAGDP_DESIGNER_ARM 0x23B

//misc
#define CC3200_JTAGDP_OKFAULT 0x02
#define CC3200_JTAGDP_WAIT 0x01
#define CC3200_JTAGDP_ACC_RETRIES 5
#define CC3200_JTAGDP_PWRUP_RETRIES 50

//JTAG-DP registers
#define CC3200_JTAGDP_REG_CSR 0x04
#define CC3200_JTAGDP_REG_ABORT 0x00

//control/status register
#define CC3200_JTAGDP_CSYSPWRUPACK (1<<31)
#define CC3200_JTAGDP_CSYSPWRUPREQ (1<<30)
#define CC3200_JTAGDP_CDBGPWRUPACK (1<<29)
#define CC3200_JTAGDP_CDBGPWRUPREQ (1<<28)
#define CC3200_JTAGDP_CDBGRSTACK (1<<27)
#define CC3200_JTAGDP_CDBGRSTREQ (1<<26)
#define CC3200_JTAGDP_TRNCNT_OFFSET 12
#define CC3200_JTAGDP_TRNCNT_MASK (0x3FF<<CC3200_JTAGDP_TRNCNT_OFFSET)
#define CC3200_JTAGDP_MASKLANE_OFFSET 8
#define CC3200_JTAGDP_MASKLANE_MASK (0xF<<CC3200_JTAGDP_MASKLANE_OFFSET)
#define CC3200_JTAGDP_WDATAERR (1<<7)
#define CC3200_JTAGDP_READOK (1<<6)
#define CC3200_JTAGDP_STICKYERR (1<<5)
#define CC3200_JTAGDP_STICKYCMP (1<<4)
#define CC3200_JTAGDP_TRNMODE_OFFSET 2
#define CC3200_JTAGDP_TRNMODE_MASK (0x03<<CC3200_JTAGDP_TRNMODE_OFFSET)
#define CC3200_JTAGDP_STICKYORUN (1<<1)
#define CC3200_JTAGDP_ORUNDETECT (1)

//MEM-AP
#define CC3200_JTAGDP_AP_IDCODE_BANK 0xF
#define CC3200_JTAGDP_AP_IDCODE_REG 0x0C

//this struct represents an AP connected to the JTAG-DP.
struct cc3200_jtagdp_ap{
    uint32_t idcode;
    uint8_t type;
    uint8_t variant;
    uint8_t is_mem_ap;
    uint8_t JEP106_id;
    uint8_t JEP106_cont;
    uint8_t revision;
};

//struct for caching status of register(s).
struct cc3200_jtagdp_cache{
    uint8_t ir;
    uint8_t cache_ready;
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
    struct cc3200_jtagdp_cache cache;
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
    .VERSION = 0,
    .cache = {
        .ir = 0,
        .cache_ready = 0
    }
};

//helper function prototypes
int cc3200_jtagdp_accResponseWrite(uint8_t* response, enum jtag_state_scan toState);
int cc3200_jtagdp_accResponseRead(uint8_t* response, uint32_t* value, enum jtag_state_scan toState);

int cc3200_jtagdp_init(int num_precede_ir_bits, uint64_t precede_ir_bits, int num_precede_dr_bits, uint64_t precede_dr_bits)
{
    cc3200_jtagdp_state.num_precede_ir_bits = num_precede_ir_bits;
    cc3200_jtagdp_state.precede_ir_bits = precede_ir_bits;
    cc3200_jtagdp_state.num_precede_dr_bits = num_precede_dr_bits;
    cc3200_jtagdp_state.precede_dr_bits = precede_dr_bits;
    cc3200_jtagdp_state.detected = 0;
    cc3200_jtagdp_state.initialized = 1;
    cc3200_jtagdp_state.cache.cache_ready = 0;
    return RET_SUCCESS;
}

int cc3200_jtagdp_deinit(void)
{
    cc3200_jtagdp_state.initialized = 0;
    return RET_SUCCESS;
}

int cc3200_jtagdp_detect(void)
{
    if(!cc3200_jtagdp_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    //read the IDCODE of the core JTAG debug port.
    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_IDCODE,
            JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift IR fail");}
    if(cc3200_jtagdp_shiftDR(0, CC3200_JTAGDP_IDCODE_LEN,
            JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift DR fail");}
    uint32_t idcode = (uint32_t) (jtag_scan_getShiftOut() & 0xFFFFFFFF);

    //validate IDCODE
    cc3200_jtagdp_state.DESIGNER = (idcode>>1) & 0x7FF;
    cc3200_jtagdp_state.PARTNO = (idcode>>12) & 0xFFFF;
    cc3200_jtagdp_state.VERSION = (idcode>>28) & 0xF;
    if(cc3200_jtagdp_state.DESIGNER != CC3200_JTAGDP_DESIGNER_ARM) {RETURN_ERROR(ERROR_UNKNOWN, "Wrong IDCODE");}

    cc3200_jtagdp_state.detected = 1;
    return RET_SUCCESS;
}

int cc3200_jtagdp_selectAPBank(uint8_t AP, uint8_t bank)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    uint32_t APSELECT = (bank<<4) | AP<<24;

    if(cc3200_jtagdp_DPACC_write(0x08,APSELECT,1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "DPACC fail");}

    return RET_SUCCESS;
}

int cc3200_jtagdp_readAPs(void)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    for(uint32_t i=0; i<16; i++){
        if(cc3200_jtagdp_selectAPBank(i, CC3200_JTAGDP_AP_IDCODE_BANK) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "AP Bank sel fail");}
        if(cc3200_jtagdp_APACC_read(CC3200_JTAGDP_AP_IDCODE_REG,
                &cc3200_jtagdp_state.ap[i].idcode,1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "APACC read fail");}

        cc3200_jtagdp_state.ap[i].type = cc3200_jtagdp_state.ap[i].idcode & (0xF);
        cc3200_jtagdp_state.ap[i].variant = (cc3200_jtagdp_state.ap[i].idcode >> 4) & (0xF);
        cc3200_jtagdp_state.ap[i].is_mem_ap = (cc3200_jtagdp_state.ap[i].idcode >> 16) & (1);
        cc3200_jtagdp_state.ap[i].JEP106_id = (cc3200_jtagdp_state.ap[i].idcode >> 17) & (0x7F);
        cc3200_jtagdp_state.ap[i].JEP106_cont = (cc3200_jtagdp_state.ap[i].idcode >> 24) & (0xF);
        cc3200_jtagdp_state.ap[i].revision = (cc3200_jtagdp_state.ap[i].idcode >> 28) & (0xF);
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_shiftIR(uint8_t data, enum jtag_state_scan toState)
{
    if(!cc3200_jtagdp_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    if((data == cc3200_jtagdp_state.cache.ir) && cc3200_jtagdp_state.cache.cache_ready){
        return RET_SUCCESS;
    }

    if(jtag_scan_shiftIR((cc3200_jtagdp_state.precede_ir_bits << CC3200_JTAGDP_IR_LEN) | (data&0xF),
            cc3200_jtagdp_state.num_precede_ir_bits + CC3200_JTAGDP_IR_LEN,
            toState) == RET_FAILURE){
        {RETURN_ERROR(ERROR_UNKNOWN, "Shift IR fail");}
    }

    cc3200_jtagdp_state.cache.ir = data;
    cc3200_jtagdp_state.cache.cache_ready = 1;

    return RET_SUCCESS;
}

int cc3200_jtagdp_shiftDR(uint64_t data, int DR_len, enum jtag_state_scan toState)
{
    if(!cc3200_jtagdp_state.initialized) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    return jtag_scan_shiftDR((cc3200_jtagdp_state.precede_dr_bits << DR_len) | data,
            cc3200_jtagdp_state.num_precede_dr_bits + DR_len,
            toState);
}

int cc3200_jtagdp_accResponseWrite(uint8_t* response, enum jtag_state_scan toState)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    uint64_t shift_command = 1 | (0x0C >> 1); //dummy command

    if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
            toState) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift DR fail");}

    uint64_t result = jtag_scan_getShiftOut();
    *response = (uint8_t)(result & 0x07);

    if( (*response != CC3200_JTAGDP_WAIT) && (*response != CC3200_JTAGDP_OKFAULT) ) {RETURN_ERROR(ERROR_UNKNOWN, "Acc response fail");} //invalid response

    return RET_SUCCESS;
}

int cc3200_jtagdp_accResponseRead(uint8_t* response, uint32_t* value, enum jtag_state_scan toState)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    uint64_t shift_command = 1 | (0x0C >> 1); //dummy command

    if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_APACC_LEN,
            toState) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift DR fail");}

    uint64_t result = jtag_scan_getShiftOut();
    *response = (uint8_t)(result & 0x07);
    *value = (uint32_t)(result >> 3);

    if( (*response != CC3200_JTAGDP_WAIT) && (*response != CC3200_JTAGDP_OKFAULT) ) {RETURN_ERROR(ERROR_UNKNOWN, "Acc response fail");} //invalid response

    return RET_SUCCESS;
}

int cc3200_jtagdp_DPACC_write(uint8_t addr, uint32_t value, uint8_t check_response)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    uint64_t shift_command = ((addr&0x0C) >> 1) | ((uint64_t) value) << 3;
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_DPACC,
            JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift IR fail");}

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
                JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift DR fail");}

        if(check_response){
            if(cc3200_jtagdp_accResponseWrite(&response,
                    JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Acc response fail");}
        }
    }

    uint32_t csr;
    cc3200_jtagdp_checkCSR(&csr);
    if(csr & CC3200_JTAGDP_STICKYERR){
        //clear the sticky flag
        cc3200_jtagdp_clearCSR();
        RETURN_ERROR(csr, "JTAG Sticky Error");
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_APACC_pipeline_read(uint8_t addr, uint32_t len, uint32_t* dst)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}
    if(len == 0) return RET_SUCCESS;

    uint64_t shift_command, result;
    uint8_t response;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_APACC,
            JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift IR fail");}


    shift_command = 1 | ((addr&0x0C) >> 1);
    response = CC3200_JTAGDP_WAIT;
    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_APACC_LEN,
                JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift DR fail");}
        result = jtag_scan_getShiftOut();
        response = (uint8_t)(result & 0x07);
        if( (response != CC3200_JTAGDP_WAIT) && (response != CC3200_JTAGDP_OKFAULT) ) {RETURN_ERROR(ERROR_UNKNOWN, "Acc response fail");} //invalid response
    }
    if(response == CC3200_JTAGDP_WAIT){
        LOG(LOG_VERBOSE, "[CC3200] Pipelined read: number of WAIT retries exceeded.");
        RETURN_ERROR(ERROR_UNKNOWN, "WAIT fail");
    }

    for(uint32_t j = 0; j<len; j++){
        response = CC3200_JTAGDP_WAIT;
        for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
            if(cc3200_jtagdp_accResponseRead(&response, &(dst[j]), JTAG_STATE_SCAN_PAUSE) == RET_FAILURE){
                RETURN_ERROR(ERROR_UNKNOWN, "Acc response fail");
            }
            if( (response != CC3200_JTAGDP_WAIT) && (response != CC3200_JTAGDP_OKFAULT) ) {RETURN_ERROR(ERROR_UNKNOWN, "Acc response fail");} //invalid response
        }
        if(response == CC3200_JTAGDP_WAIT){
            LOG(LOG_VERBOSE, "[CC3200] Pipelined read: number of WAIT retries exceeded.");
            RETURN_ERROR(ERROR_UNKNOWN, "WAIT fail");
        }
    }

    uint32_t csr;
    cc3200_jtagdp_checkCSR(&csr);
    if(csr & CC3200_JTAGDP_STICKYERR) {
        LOG(LOG_IMPORTANT, "JTAG Sticky Error");
        //clear the sticky flag
        cc3200_jtagdp_clearCSR();
        RETURN_ERROR(csr, "JTAG Sticky Error");
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_APACC_pipeline_write(uint8_t addr, uint32_t len, uint32_t* values, uint8_t check_response)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}
    if(len == 0) return RET_SUCCESS;

    uint64_t shift_command, result;
    uint8_t response;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_APACC,
            JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift IR fail");}


    for(uint32_t j = 0; j<len; j++){
        shift_command = ((addr&0x0C) >> 1) | ((uint64_t)(values[j])) << 3;
        response = CC3200_JTAGDP_WAIT;

        for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && (response == CC3200_JTAGDP_WAIT); i++){
            if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_APACC_LEN,
                    JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift DR fail");}

            result = jtag_scan_getShiftOut();
            response = (uint8_t)(result & 0x07);

            if( (response != CC3200_JTAGDP_WAIT) && (response != CC3200_JTAGDP_OKFAULT) ) {RETURN_ERROR(ERROR_UNKNOWN, "Acc response fail");} //invalid response
        }
        if(response == CC3200_JTAGDP_WAIT){
            LOG(LOG_VERBOSE, "[CC3200] Pipelined write: number of WAIT retries exceeded.");
            RETURN_ERROR(ERROR_UNKNOWN, "WAIT fail");
        }
    }
    if(check_response){
        //final response check
        if(cc3200_jtagdp_accResponseWrite(&response,
                JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Acc response fail");}
    }

    uint32_t csr;
    cc3200_jtagdp_checkCSR(&csr);
    if(csr & CC3200_JTAGDP_STICKYERR) {
        LOG(LOG_IMPORTANT, "[CC3200] JTAG Sticky Error @ 0x%8X", addr);
        //clear the sticky flag
        cc3200_jtagdp_clearCSR();
        RETURN_ERROR(csr, "JTAG Sticky Error");
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_DPACC_read(uint8_t addr, uint32_t* result, uint8_t check_response)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    uint64_t shift_command = 1 | ((addr&0x0C) >> 1);
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_DPACC,
            JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift IR fail");}

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
                JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift DR fail");}

        if(check_response){
            if(cc3200_jtagdp_accResponseRead(&response, result,
                    JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Acc response fail");}
        }
    }

    uint32_t csr;
    cc3200_jtagdp_checkCSR(&csr);
    if(csr & CC3200_JTAGDP_STICKYERR) {
        //clear the sticky flag
        cc3200_jtagdp_clearCSR();
        RETURN_ERROR(csr, "JTAG Sticky Error");
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_APACC_write(uint8_t addr, uint32_t value, uint8_t check_response)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    uint64_t shift_command = ((addr&0x0C) >> 1) | ((uint64_t) value) << 3;
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_APACC,
            JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift IR fail");}

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_APACC_LEN,
                JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift DR fail");}

        if(check_response){
            if(cc3200_jtagdp_accResponseWrite(&response,
                    JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Acc response fail");}
        }
    }

    uint32_t csr;
    cc3200_jtagdp_checkCSR(&csr);
    if(csr & CC3200_JTAGDP_STICKYERR){
        //clear the sticky flag
        cc3200_jtagdp_clearCSR();
        RETURN_ERROR(csr, "JTAG Sticky Error");
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_APACC_read(uint8_t addr, uint32_t* result, uint8_t check_response)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    uint64_t shift_command = 1 | ((addr&0x0C) >> 1);
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_APACC,
            JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift IR fail");}

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_APACC_LEN,
                JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift DR fail");}

        if(check_response){
            if(cc3200_jtagdp_accResponseRead(&response, result,
                    JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Acc response fail");}
        }
    }

    uint32_t csr;
    cc3200_jtagdp_checkCSR(&csr);
    if(csr & CC3200_JTAGDP_STICKYERR){
        //clear the sticky flag
        cc3200_jtagdp_clearCSR();
        RETURN_ERROR(csr, "JTAG Sticky Error");
    }

    return RET_SUCCESS;
}

int cc3200_jtagdp_powerUpDebug(void)
{
    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    if(cc3200_jtagdp_DPACC_write(0x04, CC3200_JTAGDP_CDBGPWRUPREQ | CC3200_JTAGDP_CSYSPWRUPREQ, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "DPACC write fail");}

    uint32_t temp;
    for(int i=0; i<CC3200_JTAGDP_PWRUP_RETRIES; i++){
        if(cc3200_jtagdp_DPACC_read(0x04, &temp, 1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "DPACC read fail");}
        if((temp & CC3200_JTAGDP_CDBGPWRUPACK) && (temp & CC3200_JTAGDP_CSYSPWRUPACK)) return RET_SUCCESS;
        LOG(LOG_VERBOSE, "[CC3200 JTAG_DP] CSR value waiting for dbg powerup: 0x%8X", (unsigned int)temp);
    }

    {RETURN_ERROR(ERROR_UNKNOWN, "DPACC read fail");}

    return RET_FAILURE;
}

int cc3200_jtagdp_clearCSR(void)
{
    if(cc3200_jtagdp_DPACC_write(0x04, CC3200_JTAGDP_STICKYERR
            | CC3200_JTAGDP_STICKYCMP
            | CC3200_JTAGDP_STICKYORUN,
            1) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "CSR clear fail");}

    return RET_SUCCESS;
}

int cc3200_jtagdp_checkCSR(uint32_t* csr){

    if(!cc3200_jtagdp_state.initialized || !cc3200_jtagdp_state.detected) {RETURN_ERROR(ERROR_UNKNOWN, "Uninit fail");}

    uint64_t shift_command = 1 | ((0x04) >> 1);
    uint8_t response = CC3200_JTAGDP_WAIT;

    if(cc3200_jtagdp_shiftIR(CC3200_JTAGDP_IR_DPACC,
            JTAG_STATE_SCAN_PAUSE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift IR fail");}

    for(int i = 0; (i<CC3200_JTAGDP_ACC_RETRIES) && response == CC3200_JTAGDP_WAIT; i++){
        if(cc3200_jtagdp_shiftDR(shift_command, CC3200_JTAGDP_DPACC_LEN,
                JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Shift DR fail");}

        if(cc3200_jtagdp_accResponseRead(&response, csr,
                JTAG_STATE_SCAN_RUNIDLE) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Acc response fail");}
    }

    return RET_SUCCESS;
}
