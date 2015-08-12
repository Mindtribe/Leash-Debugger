/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#include "cc3200_core.h"
#include "cc3200_jtagdp.h"
#include "common.h"
#include "error.h"
#include "mem_log.h"

struct cc3200_core_state_t{
    int initialized;
    int detected;
    uint32_t AP_IDR;
    uint8_t device_enabled;
    uint8_t secure_priv_enabled;
    uint8_t debug_sw_enabled;
};
struct cc3200_core_state_t cc3200_core_state = {
    .initialized = 0,
    .detected = 0,
    .AP_IDR = 0,
    .device_enabled = 0,
    .secure_priv_enabled = 0,
    .debug_sw_enabled = 0
};

int cc3200_core_init(void)
{
    if(cc3200_core_state.initialized) return RET_SUCCESS;

    cc3200_core_state.initialized = 1;
    return RET_SUCCESS;
}

int cc3200_core_detect(void)
{
    cc3200_core_state.detected = 0;
    if(!cc3200_core_state.initialized) RETURN_ERROR(ERROR_UNKNOWN);

    if(cc3200_core_read_APreg(0, CC3200_CORE_AP_IDR_ADDR, &cc3200_core_state.AP_IDR) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_core_state.AP_IDR != CC3200_CORE_AP_IDR) RETURN_ERROR(ERROR_UNKNOWN);

    uint32_t csw;
    if(cc3200_core_read_APreg(0, CC3200_CORE_AP_CSW_ADDR, &csw) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    cc3200_core_state.device_enabled = (csw & CC3200_CORE_CSW_DEVICEEN) ? 1 : 0;
    cc3200_core_state.secure_priv_enabled = (csw & CC3200_CORE_CSW_SPIDEN) ? 1 : 0;
    cc3200_core_state.debug_sw_enabled = (csw & CC3200_CORE_CSW_DBGSWENABLE) ? 1 : 0;

    cc3200_core_state.detected = 1;
    return RET_SUCCESS;
}

int cc3200_core_read_APreg(uint8_t ap, uint8_t regaddr, uint32_t* result)
{
    if(!cc3200_core_state.initialized) RETURN_ERROR(ERROR_UNKNOWN);

    if(!cc3200_jtagdp_selectAPBank(ap, (regaddr>>4) & 0xF) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_jtagdp_APACC_read(regaddr & 0xF, result) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    return RET_SUCCESS;
}

int cc3200_core_write_APreg(uint8_t ap, uint8_t regaddr, uint32_t value)
{
    if(!cc3200_core_state.initialized) RETURN_ERROR(ERROR_UNKNOWN);

    if(!cc3200_jtagdp_selectAPBank(ap, (regaddr>>4) & 0xF) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    if(cc3200_jtagdp_APACC_write(regaddr & 0xF, value) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    return RET_SUCCESS;
}
