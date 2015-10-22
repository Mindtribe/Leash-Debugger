/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef CC3200_FLASHFS_H_
#define CC3200_FLASHFS_H_

//Loads the flash stub program from a predetermined file on the debugger's filesystem, into
//the target's RAM memory, then starts it and initializes a communications channel to it.
//this only needs to be done once to enable access to flash functionality through the stub.
int cc3200_flashfs_loadstub(void);

//Open a file on the target filesystem. The flash stub must be loaded and running.
int cc3200_flashfs_open(unsigned int AccessModeAndMaxSize, unsigned char* pFileName, long* pFileHandle);

//When writing to a file on the target filesystem, the debugger automatically keeps track of the
//total CRC of all writes made. In case a new file was written, this stored value can be compared
//to the read back CRC of the file on the target filesystem, to check correctness. This function
//can be used for that.
//pFileName must indicate the same file that was just most recently written.
//result will be made nonzero if the CRC's match, otherwise zero.
int cc3200_flashfs_checkcrc(unsigned char* pFileName, unsigned int *result);

//Close a file descriptor on the target filesystem.
int cc3200_flashfs_close(int FileHdl);

//Read from a file on the target filesystem.
int cc3200_flashfs_read(int FileHdl, unsigned int Offset, unsigned char* pData, unsigned int Len);

//Write to a file on the target filesystem.
int cc3200_flashfs_write(int FileHdl, unsigned int Offset, unsigned char* pData, unsigned int Len);

//Delete a file on the target filesystem.
int cc3200_flashfs_delete(unsigned char* pFileName);

//Load a binary file from the debugger filesystem into the target RAM.
int cc3200_flashfs_load(char* pFileName);

#endif
