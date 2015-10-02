/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#ifndef ARGS_H_
#define ARGS_H_

struct command_file_open_args_t{
    unsigned int AccessModeAndMaxSize;
    unsigned char* pFileName;
    long* pFileHandle;
};

struct command_file_close_args_t{
    int FileHdl;
};

struct command_file_read_args_t{
    int FileHdl;
    unsigned int Offset;
    unsigned int Len;
    unsigned char* pData;
};

struct command_file_write_args_t{
    int FileHdl;
    unsigned int Offset;
    unsigned int Len;
    unsigned char* pData;
};

struct command_file_delete_args_t{
    unsigned char* pFileName;
};

#endif
