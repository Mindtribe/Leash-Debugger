/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "gdbserver.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <fcntl.h>

#include "FreeRTOS.h"
#include "task.h"

#include "log.h"
#include "target_al.h"

#include <ctype.h>

#include "gdb_helpers.h"
#include "breakpoint.h"
#include "error.h"
#include "special_chars.h"
#include "crc32.h"
#include "led.h"
#include "ui.h"

//#define GDBSERVER_KEEP_CHARS //for debugging: whether to keep track of chars received
#define GDBSERVER_KEEP_CHARS_NUM 2096 //for debugging: number of chars to keep track of

#define GDBSERVER_MAX_PACKET_LEN_RX 2096
#define GDBSERVER_MAX_PACKET_LEN_TX 2096
#define GDBSERVER_REPORT_MAX_PACKET_LEN 2048

#define GDBSERVER_MAX_BLOCK_ACCESS 2048
#define GDBSERVER_NUM_BKPT 256
#define GDBSERVER_POLL_INTERVAL 100

#define GDBSERVER_FILENAME_MAX 128

static const char* gdbserver_log_prefix = "[GDBSERV] ";

#ifdef GDBSERVER_KEEP_CHARS
char lastChars[GDBSERVER_KEEP_CHARS_NUM];
unsigned int curChar = 0;
#endif

enum gdbserver_packet_phase{
    PACKET_NONE = 0,
    PACKET_DATA,
    PACKET_CHECKSUM
};

#define WFD_NAME_STRING ""\
        "\n"\
        "------------------------------------------\n"\
        "Leash Debugger\n"\
        "Copyright 2015, MindTribe inc.\n"\
        "------------------------------------------\n"\
        "\n"\
        "Note: this is a work-in-progress.\n"\
        "Please see known bugs in the README before using.\n"\
                "\n"


//NOTE: keep this enum and the following const array synced so that each query type corresponds to its string.
enum gdbserver_query_name{
    QUERY_C = 0,
    QUERY_CRC,
    QUERY_FTHREADINFO,
    QUERY_STHREADINFO,
    QUERY_GETTLSADDR,
    QUERY_GETTIBADDR,
    QUERY_L,
    QUERY_OFFSETS,
    QUERY_P,
    QUERY_RCMD,
    QUERY_SEARCH,
    QUERY_SUPPORTED,
    QUERY_SYMBOL,
    QUERY_TBUFFER,
    QUERY_TFP,
    QUERY_TFV,
    QUERY_TMINFTPILEN,
    QUERY_THREADEXTRAINFO,
    QUERY_TP,
    QUERY_TSP,
    QUERY_TSV,
    QUERY_TSTATUS,
    QUERY_TV,
    QUERY_TFSTM,
    QUERY_TSSTM,
    QUERY_TSTMAT,
    QUERY_XFER,
    QUERY_ATTACHED,
    QUERY_MAX, //to count the number of entries
    QUERY_ERROR //to signal an error internally
};
const char* gdbserver_query_strings[] = {
    "C",
    "CRC",
    "fThreadInfo",
    "sThreadInfo",
    "GetTLSAddr",
    "GetTIBAddr",
    "L",
    "Offsets",
    "P",
    "Rcmd",
    "Search",
    "Supported",
    "Symbol",
    "TBuffer",
    "TfP",
    "TfV",
    "TMinFTPILen",
    "ThreadExtraInfo",
    "TP",
    "TsP",
    "TsV",
    "TStatus",
    "TV",
    "TfSTM",
    "TsSTM",
    "TSTMat",
    "Xfer",
    "Attached"
};

//NOTE: keep this enum and the following const array synced so that each query type corresponds to its string.
enum gdbserver_vcommand_name{
    VCOMMAND_FILE = 0,
    VCOMMAND_MAX,
    VCOMMAND_ERROR
};
const char* gdbserver_vcommand_strings[] = {
    "File"
};

struct fileio_state_t{
    uint8_t fileio_waiting;
    struct semihost_operation last_semihost_op;
    int count;
};

struct gdbserver_state_t{
    int initialized;
    enum gdbserver_packet_phase packet_phase;
    int awaiting_ack;
    int gdb_connected;
    int target_connected;
    char tx_packet[GDBSERVER_MAX_PACKET_LEN_TX];
    char cur_packet[GDBSERVER_MAX_PACKET_LEN_RX];
    char cur_checksum_chars[2];
    uint8_t cur_checksum;
    int cur_packet_index;
    enum stop_reason stop_reason;
    struct target_al_interface *target;
    struct breakpoint breakpoints[GDBSERVER_NUM_BKPT];
    uint8_t halted;
    uint8_t gave_info;
    struct fileio_state_t fileio_state;
    unsigned int binary_rx_counter;
    unsigned int escapechar;
    unsigned int cur_packet_len;
    unsigned int stack_watermark;
};

//disable GCC warning - braces bug 53119 in GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
struct gdbserver_state_t gdbserver_state = {
    .initialized = 0,
    .packet_phase = PACKET_NONE,
    .awaiting_ack = 0,
    .tx_packet = {0},
    .cur_packet = {0},
    .cur_checksum_chars = {0},
    .cur_packet_index = 0,
    .stop_reason = STOPREASON_UNKNOWN,
    .gdb_connected = 0,
    .breakpoints = {0},
    .halted = 0,
    .gave_info = 0,
    .fileio_state = {0},
    .binary_rx_counter = 0,
    .escapechar = 0,
    .cur_checksum = 0,
    .cur_packet_len = 0,
    .target_connected = 0,
    .stack_watermark = 0xFFFFFFFF
};
#pragma GCC diagnostic pop

static int gdbserver_processChar(void);
static int gdbserver_processPacket(void);
static void gdbserver_reset_error(int error_code, char* msg);
static void gdbserver_TransmitPacket(char* packet_data);
static void gdbserver_TransmitBinaryPacket(unsigned char* packet_data, unsigned int len);
static void gdbserver_TransmitStopReason(void);
static int gdbserver_readMemory(char* argstring);
static int gdbserver_writeMemory(char* argstring);
static int gdbserver_writeMemoryBinary(char* argstring);
static void gdbserver_Interrupt(uint8_t signal);
static int gdbserver_doMemCRC(char* argstring);
static int gdbserver_pollTarget(void);
static int gdbserver_handleHalt(void);
static int gdbserver_continue(void);
static int gdbserver_handleSemiHosting(void);
static void gdbserver_sendInfo(void);
static int gdbserver_TransmitFileIOWrite(uint8_t descriptor, char *buf, uint32_t count);
static int gdbserver_vFileOpen(char* argstring);
static int gdbserver_vFileClose(char* argstring);
static int gdbserver_vFileDelete(char* argstring);
static int gdbserver_vFilePRead(char* argstring);
static int gdbserver_vFilePWrite(char* argstring);
static int gdbserver_connectTarget(int* targetConnected);
static void gdbserver_doRcmd(char* rcmd);
static int gdbserver_replyFileIO(void);

static int gdbserver_connectTarget(int* targetConnected)
{
    if((*gdbserver_state.target->target_init)() == RET_FAILURE) {goto fail_connect;}
    if((*gdbserver_state.target->target_poll_halted)(&gdbserver_state.halted) == RET_FAILURE) {goto fail_connect;}
    if(!gdbserver_state.halted) {goto fail_connect;}

    gdbserver_state.gave_info = 0;
    gdbserver_state.fileio_state.fileio_waiting = 0;
    gdbserver_state.initialized = 1;
    SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_HALTED);
    *targetConnected = 1;
    return RET_SUCCESS;

    fail_connect:
    *targetConnected = 0;
    if((*gdbserver_state.target->target_deinit)() == RET_FAILURE) {goto error;}
    return RET_SUCCESS;

    error:
    *targetConnected = 0;
    SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
    RETURN_ERROR(ERROR_UNKNOWN, "Target connect fail");
    return RET_FAILURE;

}

int gdbserver_init(void (*pPutChar)(char), void (*pGetChar)(char*), int (*pGetCharsAvail)(void), struct target_al_interface *target)
{
    if(gdb_helpers_init(pPutChar, pGetChar, pGetCharsAvail) == RET_FAILURE) {goto error;}

    SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_INIT);

    gdbserver_state.target = target;

    return RET_SUCCESS;

    error:
    SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
    RETURN_ERROR(ERROR_UNKNOWN, "GDBServer init fail");
    return RET_FAILURE;
}

static void gdbserver_TransmitPacket(char* packet_data)
{
    //do the transmission
    uint8_t checksum = 0;
    gdb_helpers_PutChar('$');
    for(int i=0; packet_data[i] != 0; i++){
        gdb_helpers_PutChar(packet_data[i]);
        checksum += packet_data[i];
    }
    gdb_helpers_PutChar('#');
    //put the checksum
    char chksm_nibbles[3];
    sprintf(chksm_nibbles, "%02X", checksum);
    gdb_helpers_PutChar(chksm_nibbles[0]);
    gdb_helpers_PutChar(chksm_nibbles[1]);

    //update state
    gdbserver_state.awaiting_ack = 1;
    //log the packet
    LOG(LOG_VERBOSE, "%sTX: %s", gdbserver_log_prefix, packet_data);

    return;
}

static void gdbserver_TransmitBinaryPacket(unsigned char* packet_data, unsigned int len)
{
    //do the transmission
    uint8_t checksum = 0;
    gdb_helpers_PutChar('$');
    for(unsigned int i=0; i<len; i++){
        if((packet_data[i]=='}') ||
                (packet_data[i]=='#') ||
                (packet_data[i]=='$') ||
                (packet_data[i]=='*')){
            gdb_helpers_PutChar('}');
            gdb_helpers_PutChar(packet_data[i]^0x20);
            checksum += (uint8_t)'}' + (((uint8_t)packet_data[i])^0x20);
        }
        else{
            gdb_helpers_PutChar(packet_data[i]);
            checksum += packet_data[i];
        }
    }
    gdb_helpers_PutChar('#');
    //put the checksum
    char chksm_nibbles[3];
    sprintf(chksm_nibbles, "%02X", checksum);
    gdb_helpers_PutChar(chksm_nibbles[0]);
    gdb_helpers_PutChar(chksm_nibbles[1]);

    //update state
    gdbserver_state.awaiting_ack = 1;
    //log the packet
    LOG(LOG_VERBOSE, "%sTX: (Binary data)", gdbserver_log_prefix);

    return;
}

static void gdbserver_TransmitDebugMsgPacket(char* packet_data)
{
    //do the transmission
    uint8_t checksum = 0;
    gdb_helpers_PutChar('$');
    char hexval[3];
    gdb_helpers_PutChar('O');
    checksum += 'O';
    for(int i=0; packet_data[i] != 0; i++){
        sprintf(hexval, "%02X", (uint8_t)packet_data[i]);
        gdb_helpers_PutChar(hexval[0]);
        gdb_helpers_PutChar(hexval[1]);
        checksum += hexval[0];
        checksum += hexval[1];
    }
    gdb_helpers_PutChar('#');
    //put the checksum
    char chksm_nibbles[3];
    sprintf(chksm_nibbles, "%02X", checksum);
    gdb_helpers_PutChar(chksm_nibbles[0]);
    gdb_helpers_PutChar(chksm_nibbles[1]);

    //update state
    //gdbserver_state.awaiting_ack = 1;

    //log the packet
    LOG(LOG_VERBOSE, "%sTX: %s" , gdbserver_log_prefix, packet_data);

    return;
}

static void gdbserver_Interrupt(uint8_t signal)
{
    if((*gdbserver_state.target->target_halt)() == RET_FAILURE){
        gdbserver_TransmitPacket("OError halting target!");
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        ADD_ERROR(ERROR_UNKNOWN, "Halt fail")
    }
    if((*gdbserver_state.target->target_poll_halted)(&gdbserver_state.halted) == RET_FAILURE){
        gdbserver_TransmitPacket("OError halting target!");
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        ADD_ERROR(ERROR_UNKNOWN, "Poll halt fail")
    }
    if(!gdbserver_state.halted){
        gdbserver_TransmitPacket("OError halting target!");
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        ADD_ERROR(ERROR_UNKNOWN, "Halt fail")
    }
    SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_HALTED);
    gdbserver_state.stop_reason = STOPREASON_INTERRUPT;
    char reply[4];
    reply[0] = 'S';
    sprintf(&(reply[1]), "%02X", signal);
    reply[3] = 0;
    gdbserver_TransmitPacket(reply);
    return;
}

static int gdbserver_processChar(void)
{
    char c;
    gdb_helpers_GetChar(&c);

#ifdef GDBSERVER_KEEP_CHARS
    lastChars[curChar++ % GDBSERVER_KEEP_CHARS_NUM] = c;
    if(curChar%GDBSERVER_KEEP_CHARS_NUM) lastChars[curChar%GDBSERVER_KEEP_CHARS_NUM]  = 0; //zero-terminate
#endif

    if(gdbserver_state.packet_phase == PACKET_DATA){ //in data phase
        if(c == '$') {gdbserver_reset_error(c, "GDBServer: $ during packet data");}
        else if(c == '#'){
            gdbserver_state.packet_phase = PACKET_CHECKSUM; //next state
            gdbserver_state.cur_packet[gdbserver_state.cur_packet_index] = 0; //terminate current packet
            gdbserver_state.cur_packet_len = gdbserver_state.cur_packet_index;
            gdbserver_state.cur_packet_index = 0;
        }
        else if(gdbserver_state.cur_packet_index >= (GDBSERVER_MAX_PACKET_LEN_RX - 1)) {gdbserver_reset_error(ERROR_UNKNOWN, "packet too long");} //too long
        else {
            gdbserver_state.cur_packet[gdbserver_state.cur_packet_index++] = c;
            gdbserver_state.cur_checksum += (uint8_t)c;
        }
    }
    else{ //all other cases
        switch(c){
        case '-':  //NACK (retransmit request)
            if(!gdbserver_state.awaiting_ack) { break; } //don't throw an error since this will start an infinite loop with GDB acking all the time
            gdbserver_TransmitPacket(gdbserver_state.tx_packet);
            break;
        case '+': //ACK
            if(!gdbserver_state.awaiting_ack) { break; } //don't throw an error since this will start an infinite loop with GDB acking all the time
            gdbserver_state.awaiting_ack = 0; //confirm successful send
            break;
        case '$':
            if(gdbserver_state.packet_phase != PACKET_NONE) { gdbserver_reset_error(ERROR_UNKNOWN, "$ during packet!"); break; } //invalid character in packet
            gdbserver_state.packet_phase = PACKET_DATA;
            gdbserver_state.cur_checksum = 0;
            gdbserver_state.cur_packet_index = 0;
            break;
        case CHAR_END_TEXT: //CTRL-C in GDB
            gdbserver_Interrupt(SIGINT);
            if((*gdbserver_state.target->target_poll_halted)(&gdbserver_state.halted) == RET_FAILURE) { ADD_ERROR(ERROR_UNKNOWN, "Interrupt fail") }
            if(!gdbserver_state.halted) { ADD_ERROR(ERROR_UNKNOWN, "Halt fail") }
            break;
        default:
            switch(gdbserver_state.packet_phase){
            case PACKET_NONE:
                //do nothing - invalid character (junk)
                //gdbserver_reset_error(__LINE__, c); break; //invalid character at this point.
                break;
            case PACKET_CHECKSUM:
                if(!isxdigit((int)c)) { gdbserver_reset_error(c, "GDBServer: Non-hex character in checksum"); break; } //checksum is hexadecimal chars only
                if(gdbserver_state.cur_packet_index == 0) {
                    gdbserver_state.cur_checksum_chars[0] = c;
                    gdbserver_state.cur_packet_index++;
                }
                else{
                    gdbserver_state.cur_checksum_chars[1] = c;
                    gdbserver_state.packet_phase = PACKET_NONE; //finished
                    if(gdbserver_processPacket() == RET_FAILURE) gdbserver_reset_error(__LINE__, ERROR_UNKNOWN); //process the last received packet.
                }
                break;
            default:
                gdbserver_reset_error(ERROR_UNKNOWN, "Wrong packet phase"); //should never reach this point
            }
            break;
        }
    }

    return RET_SUCCESS;
}

static void gdbserver_reset_error(int error_code, char* msg)
{
    ADD_ERROR(error_code, msg);
    SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
    gdbserver_state.packet_phase = PACKET_NONE;
    gdbserver_state.cur_packet_index = 0;
    gdbserver_state.awaiting_ack = 0;
    gdbserver_state.escapechar = 0;
    gdbserver_state.binary_rx_counter = 0;
}

static enum gdbserver_query_name gdbserver_getGeneralQueryName(char* query)
{
    char queryName[20];
    int qi;
    for(qi=0; (qi<19) && (query[qi]!=':') && (query[qi]!=',') && (query[qi]!='#'); qi++){
        queryName[qi] = query[qi];
    }
    queryName[qi] = 0;

    //enumerate the query
    for(enum gdbserver_query_name i = 0; i<QUERY_MAX; i++){
        if(strcmp(queryName, (const char*)gdbserver_query_strings[i]) == 0){
            return (enum gdbserver_query_name) i; //found it
        }
    }

    return QUERY_ERROR; //wasn't able to decode
}

static enum gdbserver_vcommand_name gdbserver_getVCommandName(char* command)
{
    char commandName[20];
    int qi;
    for(qi=0; qi<19 && command[qi]!=':'; qi++){
        commandName[qi] = command[qi];
    }
    commandName[qi] = 0;

    //enumerate the query
    for(enum gdbserver_vcommand_name i = 0; i<VCOMMAND_MAX; i++){
        if(strcmp(commandName, (const char*)gdbserver_vcommand_strings[i]) == 0){
            return (enum gdbserver_vcommand_name) i; //found it
        }
    }

    return VCOMMAND_ERROR; //wasn't able to decode
}

static int gdbserver_vFileOpen(char* argstring)
{
    char filename[GDBSERVER_FILENAME_MAX];
    unsigned int flags, mode;
    int fd;
    unsigned int sl_mode;
    int retval;

    char* filename_hex = strtok(argstring, ",");
    char* flags_hex = strtok(NULL, ",");
    char* mode_hex = strtok(NULL, ",");

    if((mode_hex == NULL) || (filename_hex == NULL) || (flags_hex == NULL)){
        error_add(ERROR_UNKNOWN, "GDBServer: Incomplete argument string.");
        gdbserver_TransmitPacket("F-1,270F");
        return RET_SUCCESS;
    }

    gdb_helpers_hexStrToStr(filename_hex, filename);
    sscanf(flags_hex, "%X", &flags);
    sscanf(mode_hex, "%X", &mode);

    if(flags & O_WRONLY){ sl_mode = TARGET_FLASH_MODE_CREATEANDWRITE; }
    else{ sl_mode = TARGET_FLASH_MODE_READ; }
    retval = (*gdbserver_state.target->target_flash_fs_open)(
            sl_mode,
            filename,
            &fd);
    if(retval<0){
        gdbserver_TransmitPacket("F-1,270F");
    }
    else{
        char response[20];
        snprintf(response, 20, "F%08X",(unsigned int)fd);
        gdbserver_TransmitPacket(response);
    }
    return RET_SUCCESS;
}

static int gdbserver_vFileClose(char* argstring)
{
    unsigned int fd;
    int retval;

    char *fd_hex = strtok(argstring, ",");
    if(fd_hex == NULL){
        error_add(ERROR_UNKNOWN, "GDBServer: Incomplete argument string.");
        gdbserver_TransmitPacket("F-1,270F");
        return RET_SUCCESS;
    }

    sscanf(fd_hex, "%X", &fd);

    retval = (*gdbserver_state.target->target_flash_fs_close)((int)fd);
    if(retval<0){
        gdbserver_TransmitPacket("F-1,270F");
    }
    else{
        gdbserver_TransmitPacket("F0");
    }
    return RET_SUCCESS;
}

static int gdbserver_vFileDelete(char* argstring)
{
    char filename[GDBSERVER_FILENAME_MAX];
    int retval;

    char* filename_hex = strtok(argstring, ",");

    if(filename_hex == NULL) {
        error_add(ERROR_UNKNOWN, "GDBServer: Incomplete argument string.");
        gdbserver_TransmitPacket("F-1,270F");
        return RET_SUCCESS;
    }
    gdb_helpers_hexStrToStr(filename_hex, filename);

    retval = (*gdbserver_state.target->target_flash_fs_delete)(
            filename);
    if(retval<0){
        gdbserver_TransmitPacket("F-1,270F");
    }
    else{
        gdbserver_TransmitPacket("F0");
    }
    return RET_SUCCESS;
}

static int gdbserver_vFilePRead(char* argstring)
{
    unsigned int fd;
    unsigned int count;
    unsigned int offset;
    int retval;

    char *fd_hex = strtok(argstring, ",");
    char *count_hex = strtok(NULL, ",");
    char *offset_hex = strtok(NULL, ",");
    if((offset_hex == NULL) || (count_hex == NULL) || (fd_hex == NULL)){
        error_add(ERROR_UNKNOWN, "GDBServer: Incomplete argument string.");
        gdbserver_TransmitPacket("F-1,270F");
        return RET_SUCCESS;
    }

    sscanf(fd_hex, "%X", &fd);
    sscanf(count_hex, "%X", &count);
    sscanf(offset_hex, "%X", &offset);

    if(count > (GDBSERVER_MAX_PACKET_LEN_TX - 10)){ //truncate to max packet length
        count = (GDBSERVER_MAX_PACKET_LEN_TX - 10);
    }
    retval = (*gdbserver_state.target->target_flash_fs_read)(
            (int)fd,
            offset,
            (unsigned char*)&(gdbserver_state.tx_packet[10]),
            count);
    if(retval < 0) {
        gdbserver_TransmitPacket("F-1,270F");
        return RET_SUCCESS;
    }
    if((unsigned int)retval > count){ retval = (int)count; }
    sprintf(gdbserver_state.tx_packet, "F%08X", (unsigned int)retval);
    gdbserver_state.tx_packet[9] = ';'; //this one separately to overwrite sprintf's trailing 0
    gdbserver_TransmitBinaryPacket((unsigned char*)gdbserver_state.tx_packet, retval+10);

    return RET_SUCCESS;
}

static int gdbserver_vFilePWrite(char* argstring)
{
    unsigned int fd;
    unsigned int offset;
    unsigned char *data;
    int retval;

    char *fd_hex = strtok(argstring, ",");
    char *offset_hex = strtok(NULL, ",");
    if((offset_hex == NULL) || (fd_hex == NULL)){
        error_add(ERROR_UNKNOWN, "GDBServer: Incomplete argument string.");
        gdbserver_TransmitPacket("F-1,270F");
        return RET_SUCCESS;
    }

    sscanf(fd_hex, "%X", &fd);
    sscanf(offset_hex, "%X", &offset);

    //binary data
    unsigned int binary_offset = strlen(fd_hex) + strlen(offset_hex) + 2;
    data = (unsigned char*)&(argstring[binary_offset]);
    unsigned int len = gdb_helpers_deEscape_Binary_inputLen(data,
            gdbserver_state.cur_packet_len - binary_offset);

    LOG(LOG_VERBOSE, "Writing %04X bytes", len);
    retval = (*gdbserver_state.target->target_flash_fs_write)(
            (int)fd,
            offset,
            data,
            len);
    if(retval < 0){
        gdbserver_TransmitPacket("F-1,270F");
        return RET_SUCCESS;
    }
    char* response = gdbserver_state.tx_packet;
    snprintf(response, 20, "F%08X", (unsigned int)retval);
    gdbserver_TransmitPacket(response);

    return RET_SUCCESS;
}

static int gdbserver_processVCommand(char* commandString)
{
    enum gdbserver_vcommand_name qname = gdbserver_getVCommandName(commandString);
    if(qname == VCOMMAND_ERROR){
        LOG(LOG_VERBOSE, "%s Unknown 'v' packet: 'v%s'.", gdbserver_log_prefix, commandString);
        gdbserver_TransmitPacket("");
        return RET_SUCCESS;
    }

    int retval;
    unsigned int string_index = 0;

    switch(qname){
    case VCOMMAND_FILE:
        retval = (*gdbserver_state.target->target_flash_fs_supported)();
        if(retval == TARGET_FLASH_FS_UNSUPPORTED){
            gdbserver_TransmitPacket("");
            return RET_SUCCESS;
        }

        char * tok = strtok(commandString, ":");
        string_index += strlen(tok) + 1;
        tok = strtok(NULL, ":");
        string_index += strlen(tok) + 1;
        if(tok == NULL) { RETURN_ERROR(ERROR_UNKNOWN, "Arg fail"); }

        if(strcmp(tok, "open") == 0){
            return gdbserver_vFileOpen(&(commandString[string_index]));
        }
        else if(strcmp(tok, "close") == 0){
            return gdbserver_vFileClose(&(commandString[string_index]));
        }
        else if(strcmp(tok, "pread") == 0){
            return gdbserver_vFilePRead(&(commandString[string_index]));
        }
        else if(strcmp(tok, "pwrite") == 0){
            return gdbserver_vFilePWrite(&(commandString[string_index]));
        }
        else if(strcmp(tok, "unlink") == 0){
            return gdbserver_vFileDelete(&(commandString[string_index]));
        }
        else {
            gdbserver_TransmitPacket("");
        }

        break;
    default:
        gdbserver_TransmitPacket(""); //GDB reads this as "unsupported packet"
        break;//unsupported query as of now
    }

    return RET_SUCCESS;
}

static int gdbserver_processGeneralQuery(char* queryString)
{
    enum gdbserver_query_name qname = gdbserver_getGeneralQueryName(queryString);
    if(qname == QUERY_ERROR){
        LOG(LOG_VERBOSE, "%sUnknown 'q' packet: 'q%s'.", gdbserver_log_prefix, queryString);
        gdbserver_TransmitPacket("");
        return RET_SUCCESS;
    }

    char charbuf[80];
    switch(qname){
    case QUERY_SUPPORTED: //ask whether certain packet types are supported
        sprintf(charbuf, "PacketSize=%04X", GDBSERVER_REPORT_MAX_PACKET_LEN);
        gdbserver_TransmitPacket(charbuf);
        break;
    case QUERY_TSTATUS: //ask whether a trace experiment is currently running.
        gdbserver_TransmitPacket("");
        //gdbserver_TransmitPacket("T0;tnotrun:0");
        break;
    case QUERY_TFV: //ask about current trace variables.
        //traces are unsupported, empty response
        gdbserver_TransmitPacket("");
        break;
    case QUERY_C: //ask what the current thread ID is
        gdbserver_TransmitPacket(""); //reply "unsupported"
        //gdbserver_TransmitPacket("QC-1"); //reply "all threads"
        break;
    case QUERY_ATTACHED: //ask whether the debugger attached to an existing process, or created a new one.
        //default to "attached" (this informs GDB that some stuff was already going on)
        gdbserver_TransmitPacket("1");
        break;
    case QUERY_CRC: //get a CRC checksum of memory region to verify memory
        if(gdbserver_doMemCRC(&(queryString[4])) == RET_FAILURE) {
            ADD_ERROR(ERROR_UNKNOWN, "CRC fail");
        }
        break;
    case QUERY_RCMD: //send a general, custom command to the debugger.
        gdbserver_doRcmd(queryString);
        break;
    default:
        gdbserver_TransmitPacket(""); //GDB reads this as "unsupported packet"
        break;//unsupported query as of now
    }

    return RET_SUCCESS;
}

static void gdbserver_doRcmd(char* queryString)
{
    //format of the query string is:
    //Rcmd,command#
    char* rcmd_header = strtok(queryString, ",");
    char* rcmd_hexbody = strtok(NULL, "#");

    if(strcmp(rcmd_header, "Rcmd") != 0){
        LOG(LOG_VERBOSE, "%s Invalid Rcmd.", gdbserver_log_prefix);
        gdbserver_TransmitPacket("E00");
        return;
    }
    char rcmd_body[40];
    gdb_helpers_hexStrToStr(rcmd_hexbody, rcmd_body);
    LOG(LOG_VERBOSE, "%s Rcmd: %s", gdbserver_log_prefix, rcmd_body);

    int retval = (*gdbserver_state.target->target_rcmd)(rcmd_body, &gdbserver_TransmitDebugMsgPacket);
    if(retval == RET_SUCCESS) {
        gdbserver_TransmitPacket("OK");
    }
    else{
        gdbserver_TransmitPacket("E00");
    }
    return;
}

static void gdbserver_sendInfo(void)
{
    gdbserver_TransmitDebugMsgPacket(WFD_NAME_STRING);

    return;
}

static int gdbserver_processPacket(void)
{
    //check the checksum
    unsigned int checksum_parse;
    sscanf(gdbserver_state.cur_checksum_chars, "%02X", &checksum_parse);
    if((uint8_t)checksum_parse != gdbserver_state.cur_checksum){
        gdbserver_reset_error(gdbserver_state.cur_checksum, "GDBServer: Checksum mismatch.");
        gdb_helpers_Nack();
        return RET_SUCCESS;
    }

    //acknowledge
    gdb_helpers_Ack();

    //log the packet
    LOG(LOG_VERBOSE, "%s RX: %s", gdbserver_log_prefix , gdbserver_state.cur_packet);

    //process the packet based on header character
    switch(gdbserver_state.cur_packet[0]){
    case 'q': //general query
        if(gdbserver_processGeneralQuery(&(gdbserver_state.cur_packet[1])) == RET_FAILURE) ADD_ERROR(ERROR_UNKNOWN, "Parse fail")
        break;
    case 'v': //v category of commands
        if(gdbserver_processVCommand(&(gdbserver_state.cur_packet[1])) == RET_FAILURE) ADD_ERROR(ERROR_UNKNOWN, "Parse fail")
        break;
    case 'H': //set operation type and thread ID
        if((gdbserver_state.cur_packet[1] != 'c' && gdbserver_state.cur_packet[1] != 'g') //operation types
                || (gdbserver_state.cur_packet[2] != '0' && (gdbserver_state.cur_packet[2] != '-' || gdbserver_state.cur_packet[3] != '1'))){ //thread 0 or -1 only
            ADD_ERROR(ERROR_UNKNOWN, "Thread fail");
            gdbserver_TransmitPacket("E1"); //reply error
            break;
        }
        gdbserver_TransmitPacket("OK"); //reply OK
        break;
    case '?': //ask for the reason why execution has stopped
        gdbserver_TransmitStopReason();
        break;
    case 'g': //request the register status
        if((*gdbserver_state.target->target_get_gdb_reg_string)(gdbserver_state.tx_packet)
                == RET_FAILURE){
            ADD_ERROR(ERROR_UNKNOWN, "Reg get fail");
            gdbserver_TransmitPacket("E00");
        }
        else{ gdbserver_TransmitPacket(gdbserver_state.tx_packet); } //send the register string
        break;
    case 'G': //write registers
        strtok(gdbserver_state.cur_packet, "#"); //0-terminate the string
        if((*gdbserver_state.target->target_put_gdb_reg_string)(&(gdbserver_state.cur_packet[1]))
                == RET_FAILURE){
            ADD_ERROR(ERROR_UNKNOWN, "Reg put fail");
            gdbserver_TransmitPacket("E00");
        }
        else {gdbserver_TransmitPacket("OK");}
        break;
    case 'm': //read memory
        if(gdbserver_readMemory(&(gdbserver_state.cur_packet[1])) == RET_FAILURE){
            gdbserver_TransmitPacket("E00");
            ADD_ERROR(ERROR_UNKNOWN, "Mem read fail");
        }
        break;
    case 'M': //write memory
        if(gdbserver_writeMemory(&(gdbserver_state.cur_packet[1])) == RET_FAILURE){
            gdbserver_TransmitPacket("E00");
            ADD_ERROR(ERROR_UNKNOWN, "Mem write fail");
        }
        break;
    case 'X': //write memory (binary data)
        if(gdbserver_writeMemoryBinary(&(gdbserver_state.cur_packet[1])) == RET_FAILURE){
            gdbserver_TransmitPacket("E00");
            ADD_ERROR(ERROR_UNKNOWN, "Bin write fail");
        }
        break;
    case 'c': //continue
        if(strlen(gdbserver_state.cur_packet) != 1){
            //there is an address to go to. unsupported
            ADD_ERROR(ERROR_UNKNOWN, "Continue fail");
            gdbserver_TransmitPacket("E00");
        }
        if(gdbserver_continue() == RET_FAILURE){
            ADD_ERROR(ERROR_UNKNOWN, "Continue fail");
        }
        break;
    case 's': //single step
        if(strlen(gdbserver_state.cur_packet) != 1){
            //there is an address to go to. unsupported
            ADD_ERROR(ERROR_UNKNOWN, "Single step fail");
            gdbserver_TransmitPacket("E00");
        }
        if((*gdbserver_state.target->target_step)()
                == RET_FAILURE){
            ADD_ERROR(ERROR_UNKNOWN, "Single step fail");
            gdbserver_TransmitPacket("E00");
        }
        gdbserver_state.halted = 1; //still halted
        char reply[4];
        sprintf(reply, "S%02X", SIGTRAP);
        gdbserver_TransmitPacket(reply);
        break;
    case 'F': //reply to File I/O
        gdbserver_replyFileIO();
        break;
    case 'k': //kill request - we interpret this to be a system reset.
        if((*gdbserver_state.target->target_reset)() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Reset fail");}
        gdbserver_state.halted = 0; //assume we are no longer halted after reset
        break;
    default:
        gdbserver_TransmitPacket(""); //GDB reads this as "unsupported packet"
        break;
    }

    if(!gdbserver_state.gdb_connected){
        gdbserver_state.gdb_connected = 1;
    }

    return RET_SUCCESS;
}

static int gdbserver_replyFileIO(void)
{
    if(!gdbserver_state.fileio_state.fileio_waiting) ADD_ERROR(ERROR_UNKNOWN, "FileIO fail");
    gdbserver_state.fileio_state.fileio_waiting = 0;

    int continue_exec = 1;
    int num_return_args = 1;
    int arg_locations[3] = {0};
    int cur_arg = 1;

    //scan for comma argument separators and replace them with 0's
    for(int i=0; ;i++){
        if(gdbserver_state.cur_packet[i+1] == ','){
            gdbserver_state.cur_packet[i+1] = 0;
            arg_locations[cur_arg++] = i+1;
            num_return_args++;
        }
        else if(gdbserver_state.cur_packet[i+1] == ';' ||
                gdbserver_state.cur_packet[i+1] == 0 ||
                gdbserver_state.cur_packet[i+1] == '#'){
            gdbserver_state.cur_packet[i+1] = 0;
            break;
        }
    }

    int return_code;
    switch(gdbserver_state.fileio_state.last_semihost_op.opcode){
    case SEMIHOST_WRITECONSOLE:
        //no steps are required.
        break;
    case SEMIHOST_READCONSOLE:
        //handle first argument: return code.
        sscanf(&(gdbserver_state.cur_packet[arg_locations[0]+1]), "%X", (unsigned int*)&return_code);
        if(return_code == -1){ //error
            if((*gdbserver_state.target->target_write_register)(0, (uint32_t)return_code) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Reg write fail");}
        }
        else{ //number of bytes read
            if((*gdbserver_state.target->target_write_register)(
                    0, gdbserver_state.fileio_state.count - return_code) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Reg write fail");}
        }
        break;
    default:
        //TODO: reply
        return RET_SUCCESS;
        break;
    }
    //increment the PC to pass the BKPT instruction for continuing.
    uint32_t pc;
    if((*gdbserver_state.target->target_get_pc)(&pc) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "PC read fail");}
    if((*gdbserver_state.target->target_set_pc)(pc+2) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "PC write fail");}

    //handle 3rd argument if necessary: CTRL-C flag
    if((num_return_args >= 3) && (gdbserver_state.cur_packet[arg_locations[2]+1] == 'C')){
        continue_exec = 0;
    }
    if(continue_exec){
        gdbserver_continue();
    }
    else{
        gdbserver_TransmitPacket("S02"); //interrupt packet
    }

    return RET_SUCCESS;
}

static int gdbserver_readMemory(char* argstring)
{
    //First, get the arguments
    char* addrstring = strtok(argstring, ",");
    char* lenstring = strtok(NULL, "#");

    uint32_t addr = strtol(addrstring, NULL, 16);
    uint32_t len = strtol(lenstring, NULL, 16);
    uint8_t *data = (uint8_t*)argstring; //re-use argstring storage to save stack space

    if(len>GDBSERVER_MAX_BLOCK_ACCESS) {
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        RETURN_ERROR(ERROR_UNKNOWN, "Max access fail");
    }

    if((*gdbserver_state.target->target_mem_block_read)(addr, len, data) == RET_FAILURE) {
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        RETURN_ERROR(ERROR_UNKNOWN, "Mem read fail");
    }

    //construct the answer string
    uint32_t i;
    for(i=0; i<len; i++){
        sprintf(&(gdbserver_state.tx_packet[i*2]), "%02X", data[i]);
    }
    gdbserver_state.tx_packet[i*2] = 0; //terminate

    gdbserver_TransmitPacket(gdbserver_state.tx_packet);

    return RET_SUCCESS;
}

static int gdbserver_doMemCRC(char* argstring)
{
    //First, get the arguments
    char* addrstring = strtok(argstring, ",");
    char* lenstring = strtok(NULL, "#");

    uint32_t addr = strtol(addrstring, NULL, 16);
    uint32_t len = strtol(lenstring, NULL, 16);
    uint32_t bytes_left = len;

    unsigned long long crc = 0xFFFFFFFF;

    uint8_t data[GDBSERVER_MAX_BLOCK_ACCESS];
    while(bytes_left){
        if((*gdbserver_state.target->target_mem_block_read)(addr,
                (GDBSERVER_MAX_BLOCK_ACCESS < bytes_left) ? GDBSERVER_MAX_BLOCK_ACCESS : bytes_left,
                        data)
                        == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Mem read fail");} //get some bytes
        crc = crc32(data,
                (GDBSERVER_MAX_BLOCK_ACCESS < bytes_left) ? GDBSERVER_MAX_BLOCK_ACCESS : bytes_left,
                        crc);
        bytes_left-= (GDBSERVER_MAX_BLOCK_ACCESS < bytes_left) ? GDBSERVER_MAX_BLOCK_ACCESS : bytes_left;
        addr+=GDBSERVER_MAX_BLOCK_ACCESS;
    }

    //construct the answer string
    char retstring[10];
    sprintf(retstring, "C%02X%02X%02X%02X",
            (unsigned int) (0xFF & (crc>>24)),
            (unsigned int) (0xFF & (crc>>16)),
            (unsigned int) (0xFF & (crc>>8)),
            (unsigned int) (0xFF & (crc)));

    gdbserver_TransmitPacket(retstring);

    return RET_SUCCESS;
}

static int gdbserver_writeMemoryBinary(char* argstring)
{
    //First, get the arguments
    char* addrstring = strtok(argstring, ",");
    char* lenstring = strtok(NULL, ":");
    uint8_t* data = (uint8_t*) &(lenstring[strlen(lenstring)+1]);

    uint32_t addr = strtol(addrstring, NULL, 16);
    uint32_t len = strtol(lenstring, NULL, 16);

    gdb_helpers_deEscape_Binary_outputLen(data, len);

    if(len>GDBSERVER_MAX_BLOCK_ACCESS){
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        RETURN_ERROR(len, "GDBServer: Memory write over maximum length requested.");
    }

    if((*gdbserver_state.target->target_mem_block_write)(addr, len, data) == RET_FAILURE) {
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        RETURN_ERROR(ERROR_UNKNOWN, "Mem write fail");
    }

    gdbserver_TransmitPacket("OK");

    return RET_SUCCESS;
}

static int gdbserver_writeMemory(char* argstring)
{
    //First, get the arguments
    char* addrstring = strtok(argstring, ",");
    char* lenstring = strtok(NULL, ":");
    char* datastring = strtok(NULL, "#");

    uint32_t addr = strtol(addrstring, NULL, 16);
    uint32_t len = strtol(lenstring, NULL, 16);
    uint8_t data[GDBSERVER_MAX_BLOCK_ACCESS];

    if(len>GDBSERVER_MAX_BLOCK_ACCESS){
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        RETURN_ERROR(len, "GDBServer: Memory write over maximum length requested.");
    }

    //convert data
    for(uint32_t i=0; i<len; i++){
        unsigned int datai;
        sscanf(&(datastring[i*2]), "%02X", &datai);
        data[i] = (uint8_t) datai;
    }

    if((*gdbserver_state.target->target_mem_block_write)(addr, len, data) == RET_FAILURE) {
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        RETURN_ERROR(ERROR_UNKNOWN, "Mem write fail");
    }

    gdbserver_TransmitPacket("OK");

    return RET_SUCCESS;
}

void Task_gdbserver(void* params)
{
    for(;;){
        if(!gdbserver_state.target_connected){
            gdbserver_connectTarget(&gdbserver_state.target_connected);
            vTaskDelay(1000);
        }
        else {
            if(!gdbserver_state.halted) gdbserver_pollTarget();
            if(gdb_helpers_CharsAvaliable()){
                gdbserver_processChar();
                while(gdbserver_state.packet_phase != PACKET_NONE){
                    gdbserver_processChar();
                }
            }
        }
        //TODO: replace delay-loop polling by timer-based polling
        vTaskDelay(1);
#ifdef DO_STACK_CHECK
        gdbserver_state.stack_watermark = uxTaskGetStackHighWaterMark(NULL);
#endif
    };

    (void)params; //avoid unused warning
    return;
}


static int gdbserver_pollTarget(void)
{
    int old_halted = gdbserver_state.halted;
    if((*gdbserver_state.target->target_poll_halted)(&gdbserver_state.halted) == RET_FAILURE) {
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        RETURN_ERROR(ERROR_UNKNOWN, "Halt fail");
    }
    if(old_halted && !gdbserver_state.halted) {
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        RETURN_ERROR(ERROR_UNKNOWN, "Halt fail");
    }
    if(!old_halted && gdbserver_state.halted){
        if(gdbserver_handleHalt() == RET_FAILURE) {
            SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
            RETURN_ERROR(ERROR_UNKNOWN, "Halt handle fail");
        }
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_HALTED);
        char reply[4];
        switch(gdbserver_state.stop_reason){
        case STOPREASON_SEMIHOSTING:
            //semihosting is special: we don't report a stop to GDB, but instead
            //perform the semihosting action.
            if(gdbserver_handleSemiHosting() == RET_FAILURE){
                ADD_ERROR(ERROR_UNKNOWN, "Semihost handle fail");
            }
            break;
        case STOPREASON_BREAKPOINT:
            sprintf(reply, "S%02X", SIGTRAP);
            gdbserver_TransmitPacket(reply);
            break;
        case STOPREASON_INTERRUPT:
            sprintf(reply, "S%02X", SIGINT);
            gdbserver_TransmitPacket(reply);
            break;
        case STOPREASON_UNKNOWN:
        default:
            ADD_ERROR(ERROR_UNKNOWN, "Stop reason fail")
            break;
        }
    }
    return RET_SUCCESS;
}

static int gdbserver_handleHalt(void)
{
    return (*gdbserver_state.target->target_handleHalt)(&gdbserver_state.stop_reason);
}

static void gdbserver_TransmitStopReason(void){
    if(!gdbserver_state.halted){
        return; //don't reply anything - still running
    }

    switch(gdbserver_state.stop_reason){
    default:
    case STOPREASON_UNKNOWN:
        gdbserver_TransmitPacket("S02");
        break;
    }
}

static int gdbserver_TransmitFileIOWrite(uint8_t descriptor, char *buf, uint32_t count)
{
    //we must wait for the previous file I/O to finish
    if(gdbserver_state.fileio_state.fileio_waiting) {
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        RETURN_ERROR(ERROR_UNKNOWN, "FileIO fail");
    }

    char msg[28];
    sprintf(msg, "Fwrite,%02X,%02X%02X%02X%02X,%02X%02X%02X%02X",
            descriptor,
            (unsigned int)(0xFF & ((uint32_t)buf>>24)),
            (unsigned int)(0xFF & ((uint32_t)buf>>16)),
            (unsigned int)(0xFF & ((uint32_t)buf>>8)),
            (unsigned int)(0xFF & ((uint32_t)buf)),
            (unsigned int)(0xFF & (count>>24)),
            (unsigned int)(0xFF & (count>>16)),
            (unsigned int)(0xFF & (count>>8)),
            (unsigned int)(0xFF & (count)));

    gdbserver_TransmitPacket(msg);
    gdbserver_state.fileio_state.fileio_waiting = 1;
    gdbserver_state.fileio_state.count = count;

    return RET_SUCCESS;
}

static int gdbserver_TransmitFileIORead(uint8_t descriptor, char *buf, uint32_t count)
{
    //we must wait for the previous file I/O to finish
    if(gdbserver_state.fileio_state.fileio_waiting) {
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        RETURN_ERROR(ERROR_UNKNOWN, "FileIO fail");
    }

    char msg[28];
    sprintf(msg, "Fread,%02X,%02X%02X%02X%02X,%02X%02X%02X%02X",
            descriptor,
            (unsigned int)(0xFF & ((uint32_t)buf>>24)),
            (unsigned int)(0xFF & ((uint32_t)buf>>16)),
            (unsigned int)(0xFF & ((uint32_t)buf>>8)),
            (unsigned int)(0xFF & ((uint32_t)buf)),
            (unsigned int)(0xFF & (count>>24)),
            (unsigned int)(0xFF & (count>>16)),
            (unsigned int)(0xFF & (count>>8)),
            (unsigned int)(0xFF & (count)));

    gdbserver_TransmitPacket(msg);
    gdbserver_state.fileio_state.fileio_waiting = 1;
    gdbserver_state.fileio_state.count = count;

    return RET_SUCCESS;
}

static int gdbserver_continue(void)
{
    if(!gdbserver_state.halted) {
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        RETURN_ERROR(ERROR_UNKNOWN, "Halt fail");
    }

    if(!gdbserver_state.gave_info){
        gdbserver_sendInfo();
        gdbserver_state.gave_info = 1;
    }

    if((*gdbserver_state.target->target_continue)()
            == RET_FAILURE){
        gdbserver_TransmitPacket("E00");
        SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_FAILED);
        RETURN_ERROR(ERROR_UNKNOWN, "Continue fail");
    }
    gdbserver_state.halted = 0; //force halted off so polling starts
    SetLEDBlink(LED_3, LED_BLINK_PATTERN_JTAG_RUNNING);

    return RET_SUCCESS;
}

static int gdbserver_handleSemiHosting(void)
{
    //a semihosting operation is supposed to happen. Find out which one
    if((*gdbserver_state.target->target_querySemiHostOp)(
            &(gdbserver_state.fileio_state.last_semihost_op)) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "Semihost query fail");}

    switch(gdbserver_state.fileio_state.last_semihost_op.opcode){
    case SEMIHOST_WRITECONSOLE:
        if(gdbserver_TransmitFileIOWrite(1,
                (char*)gdbserver_state.fileio_state.last_semihost_op.param1,
                gdbserver_state.fileio_state.last_semihost_op.param2)
                == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "FileIO fail");}
        break;
    case SEMIHOST_READCONSOLE:
        if(gdbserver_TransmitFileIORead(
                gdbserver_state.fileio_state.last_semihost_op.param1,
                (char*)gdbserver_state.fileio_state.last_semihost_op.param2,
                gdbserver_state.fileio_state.last_semihost_op.param3)
                == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN, "FileIO fail");}
        break;
    default:
        //TODO: reply
        return RET_SUCCESS;
        break;
    }

    return RET_SUCCESS;
}
