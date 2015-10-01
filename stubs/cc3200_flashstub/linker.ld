/*****************************************************************************
* blinky.ld
*
* GCC Linker script for blinky application.
*
* Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

MEMORY
{
    /* SRAM size of 240KB for cc3200 ES 1.33 device onward */
    
    /*
    *  SRAM (rwx) : ORIGIN = 0x20004000, LENGTH = 0x3C000
    *  Above is the full SRAM size. For the flash stub, it is
    *  split into the stub region and the communication (shared)
    *  region.
    */
    SRAM_FLASH_STUB : ORIGIN = 0x20004000, LENGTH = 0x20000
    SRAM_FLASH_SHARED : ORIGIN = 0x20024000, LENGTH = 0x1C000
}

SECTIONS
{
    .text :
    {
        _text = .;
        KEEP(*(.intvecs))
        *(.text*)
        *(.rodata*)
	*(.ARM.extab* .gnu.linkonce.armextab.*)
	. = ALIGN(8);
        _etext = .;
    } > SRAM_FLASH_STUB

    .ARM : {
    __exidx_start = .;
      *(.ARM.exidx*)
      __exidx_end = .;
    } > SRAM_FLASH_STUB

    __init_data = .;

    .data : AT(__init_data)
    {
        _data = .;
        *(.data*)
	. = ALIGN (8);
        _edata = .;
    } > SRAM_FLASH_STUB

    .bss :
    {
        _bss = .;
	//__bss_start__ = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
	//__bss_end__ = .;
    } > SRAM_FLASH_STUB
    
    .shared :
    {
    	_shared = .;
    	*(.shared*)
    	_eshared = .;
   	} > SRAM_FLASH_SHARED
}

