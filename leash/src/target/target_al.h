/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

/*
 * For abstracting from different debug targets to generic debugging calls.
 */

#ifndef TARGET_AL_H_
#define TARGET_AL_H_

#include <stdint.h>

//Possible reasons why a target may have halted.
enum stop_reason{
    STOPREASON_UNKNOWN = 0,
    STOPREASON_INTERRUPT, //interrupt initiated by the debug adapter
    STOPREASON_BREAKPOINT, //breakpoint encountered
    STOPREASON_SEMIHOSTING //semihosting request made by target
};

//Possible semihosting requests which can be made by the target.
enum semihost_opcode{
    SEMIHOST_READCONSOLE = 0, //target wants to read from the debug console
    SEMIHOST_WRITECONSOLE, //target wants to write to the debug console
    SEMIHOST_UNKNOWN
};

enum target_flash_support{
    TARGET_FLASH_FS_UNSUPPORTED = 0,
    TARGET_FLASH_FS_SUPPORTED
};

enum target_flash_modes{
    TARGET_FLASH_MODE_READ = 1,
    TARGET_FLASH_MODE_CREATEANDWRITE = 2
};

struct semihost_operation{
    enum semihost_opcode opcode;
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
};

/*
 * target_al_interface is a struct of generic function pointers, which may
 * be implemented for various debug targets.
 */
struct target_al_interface{
    /*
     * target_init:
     * Initialize the debug target.
     * Precondition: none
     * Postcondition: if successful, the target has been detected, configured if necessary, and halted.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_init)(void);

    /*
     * target_deinit:
     * Deinitialize the debug target.
     * Precondition: target_init has been called before.
     * Postcondition: if successful, the target is in a state where target_init may be attempted again.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_deinit)(void);

    /*
     * target_reset:
     * Reset the debug target.
     * Precondition: target has been initialized and halted.
     * Postcondition: if successful, the target has undergone some form of reset. Which reset
     * this is (soft/hard, local/global) is implementation-dependent, but it is recommended
     * that it is as global and hard as possible.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_reset)(void);

    /*
     * target_halt:
     * Halt the debug target.
     * Precondition: Target has been initialized and is running.
     * Postcondition: if successful, the target has been halted.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_halt)(void);

    /*
     * target_continue:
     * Continue running the debug target.
     * Precondition: Target has been initialized and is halted.
     * Postcondition: if successful, the target is running.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_continue)(void);

    /*
     * target_step:
     * Step the target by a single instruction.
     * Precondition: Target is initialized and halted.
     * Postcondition: if successful, the target has performed a single instruction step and
     *  is again halted.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_step)(void);

    /*
     * target_mem_read:
     * Read a word from the target.
     * Precondition: Target is initialized.
     * Postcondition: none.
     *
     * Arguments:
     * uint32_t address: the address to read the word from.
     * uint32_t* data: pointer to a location to hold the read result.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_mem_read)(uint32_t, uint32_t*);

    /*
     * target_mem_write:
     * Write a word to the target.
     * Precondition: Target is initialized.
     * Postcondition: none.
     *
     * Arguments:
     * uint32_t address: the address to write the word to.
     * uint32_t data: the data to write to the address.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_mem_write)(uint32_t, uint32_t);

    /*
     * target_mem_block_read:
     * Read an arbitrary number of bytes from the target.
     * Precondition: Target is initialized.
     * Postcondition: none.
     *
     * Arguments:
     * uint32_t address: the starting address to read from.
     * uint32_t len: the amount of bytes to read.
     * uint8_t* data: the data buffer to store the bytes in.
     *
     * The bytes are subject to platform-specific endianness.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_mem_block_read)(uint32_t, uint32_t, uint8_t*);

    /*
     * target_mem_block_write:
     * Write an arbitrary number of bytes to the target.
     * Precondition: Target is initialized.
     * Postcondition: none.
     *
     * Arguments:
     * uint32_t address: the starting address to write to.
     * uint32_t len: the amount of bytes to write.
     * uint8_t* data: the data buffer containing bytes to write.
     *
     * The bytes are subject to platform-specific endianness.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_mem_block_write)(uint32_t, uint32_t, uint8_t*);

    /*
     * target_get_gdb_reg_string:
     * Read the target registers, and format them into a GDB-style
     * register string.
     * Precondition: Target is initialized.
     * Postcondition: none.
     *
     * Arguments:
     * char* regstring: string buffer in which to store the register string.
     *
     * The bytes are subject to platform-specific endianness.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_get_gdb_reg_string)(char*);

    /*
     * target_put_gdb_reg_string:
     * Write the target registers, taking the data to write from a GDB-style
     * register string.
     * Precondition: Target is initialized.
     * Postcondition: none.
     *
     * Arguments:
     * char* regstring: string buffer from which to take the values to be written.
     *
     * The bytes are subject to platform-specific endianness.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_put_gdb_reg_string)(char*);

    /*
     * target_read_register:
     * Read a single register from the target.
     * Precondition: Target is initialized.
     * Postcondition: none.
     *
     * Arguments:
     * uint32_t regnum: index value of the register to read.
     * uint32_t *data: pointer to a location to store the result.
     *
     * The bytes are subject to platform-specific endianness.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_read_register)(uint8_t, uint32_t*);

    /*
     * target_write_register:
     * Write a single register to the target.
     * Precondition: Target is initialized.
     * Postcondition: none.
     *
     * Arguments:
     * uint32_t regnum: index value of the register to write.
     * uint32_t data: data to write to the register.
     *
     * The bytes are subject to platform-specific endianness.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_write_register)(uint8_t, uint32_t);

    /*
     * target_set_pc:
     * Write to the target's program counter.
     * Precondition: Target is initialized.
     * Postcondition: none.
     *
     * Arguments:
     * uint32_t data: data to write to the program counter.
     *
     * The bytes are subject to platform-specific endianness.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_set_pc)(uint32_t);

    /*
     * target_get_pc:
     * Read the target's program counter.
     * Precondition: Target is initialized.
     * Postcondition: none.
     *
     * Arguments:
     * uint32_t *data: pointer to location to store the result.
     *
     * The bytes are subject to platform-specific endianness.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_get_pc)(uint32_t*);

    /*
     * target_set_sw_bkpt:
     * Set a software breakpoint on the target.
     * Precondition: Target is initialized and halted.
     * Postcondition: If successful, a breakpoint has been placed.
     *
     * Arguments:
     * uint32_t address: pointer to location to put the software breakpoint
     * uint8_t len: length in bytes of the breakpoint instruction to insert.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_set_sw_bkpt)(uint32_t, uint8_t);

    /*
     * target_poll_halted:
     * Check whether the target is currently halted.
     * Precondition: Target is initialized.
     * Postcondition: none.
     *
     * Arguments:
     * uint8_t *result: will be set to nonzero if the target is halted.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_poll_halted)(uint8_t*);

    /*
     * target_handleHalt:
     * Request why the target is currently halted.
     * Precondition: Target is initialized and halted.
     * Postcondition: none.
     *
     * Arguments:
     * enum stop_reason *reason: pointer to location to store the result.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_handleHalt)(enum stop_reason *);

    /*
     * target_querySemiHostOp:
     * For ARM targets only. May be called after it has been determined that the target was halted due to
     * a semihosting request (special breakpoint). This funtion will find out what kind of semihosting
     * operation was requested.
     * Precondition: Target is initialized, halted due to a semihosting request.
     * Postcondition: none.
     *
     * Arguments:
     * struct semihost_operation* op: pointer to location to store the result.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_querySemiHostOp)(struct semihost_operation *);

    /*
     * target_flash_fs_supported:
     * Query whether this target supports a remote flash filesystem.
     * Precondition: Target is initialized.
     * Postcondition: none.
     *
     * return: nonzero if supported, otherwise zero.
     */
    int (*target_flash_fs_supported)(void);

    /*
     * target_flash_fs_read:
     * Read from a file on the target's flash filesystem.
     * Precondition: Target is initialized and supports a flash filesystem. A file
     *  is currently open for reading.
     * Postcondition: The file is still open.
     *
     * Arguments:
     * int fd: file descriptor obtained through target_flash_fs_open().
     * unsigned int offset: offset in bytes from which to start reading the file.
     * unsigned char* buffer: pointer to data buffer where read data should be stored.
     * unsigned int len: number of bytes to read.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_flash_fs_read)(int, unsigned int, unsigned char*, unsigned int);

    /*
     * target_flash_fs_write:
     * Write to a file on the target's flash filesystem.
     * Precondition: Target is initialized and supports a flash filesystem. A file
     *  is currently open for writing.
     * Postcondition: The file is still open.
     *
     * Arguments:
     * int fd: file descriptor obtained through target_flash_fs_open().
     * unsigned int offset: offset in bytes from which to start writing to the file.
     * unsigned char* buffer: pointer to data buffer where data to write is stored.
     * unsigned int len: number of bytes to write.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_flash_fs_write)(int, unsigned int, unsigned char*, unsigned int);

    /*
     * target_flash_fs_open:
     * Open a file on the target's flash filesystem.
     * Precondition: Target is initialized and has a flash filesystem. No files are open.
     * Postcondition: If successful, a file is open.
     *
     * Arguments:
     * unsigned int flags: flags to open the file with. options are:
     *   TARGET_FLASH_MODE_READ (open for reading only)
     *   TARGET_FLASH_MODE_CREATEANDWRITE (open for writing only. creates if not existing, overwrites if existing).
     * char* filename: filename on the remote filesystem to open.
     * int* fd: pointer to location to store a file descriptor for the open file.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_flash_fs_open)(unsigned int, char*, int*);

    /*
     * target_flash_fs_close:
     * Close an open file on the remote filesystem.
     * Precondition: Target is initialized and has a flash filesystem. A file is open.
     * Postcondition: If successful, no files are open.
     *
     * Arguments:
     * int fd: file descriptor of the open file.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_flash_fs_close)(int);

    /*
     * target_flash_fs_delete:
     * Delete a file from the target's flash filesystem.
     * Precondition: Target is initialized and supports a flash filesystem. No files are open.
     * Postcondition: If successful, the file is deleted.
     *
     * Arguments:
     * char* filename: name of the file to delete.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_flash_fs_delete)(char*);

    /*
     * target_rcmd:
     * Send a custom command string to the target. This may be used to implement custom,
     * platform-dependent functionality.
     * Precondition: Target is initialized.
     * Postcondition: none (implementation-dependent on the command).
     *
     * Arguments:
     * char* command: command string
     * void(*pMsgCallback)(char*): pointer to a callback function. The target implementation may
     *   use this callback to send status update or reply messages back while executing the
     *   command. This function may be called back an arbitrary number of times.
     *
     * return: RET_SUCCESS if successful, otherwise RET_FAILURE.
     */
    int (*target_rcmd)(char*, void(*)(char*));
};

#endif
