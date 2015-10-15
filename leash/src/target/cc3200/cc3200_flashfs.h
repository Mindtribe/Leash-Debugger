/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (TI CC3200 - Launchpad XL)
    --------------------------------------------------------- */

#ifndef CC3200_FLASHFS_H_
#define CC3200_FLASHFS_H_

int cc3200_flashfs_loadstub(void);
int cc3200_flashfs_open(unsigned int AccessModeAndMaxSize, unsigned char* pFileName, long* pFileHandle);
int cc3200_flashfs_close(int FileHdl);
int cc3200_flashfs_read(int FileHdl, unsigned int Offset, unsigned char* pData, unsigned int Len);
int cc3200_flashfs_write(int FileHdl, unsigned int Offset, unsigned char* pData, unsigned int Len);
int cc3200_flashfs_delete(unsigned char* pFileName);
int cc3200_flashfs_load(char* pFileName);

#endif
