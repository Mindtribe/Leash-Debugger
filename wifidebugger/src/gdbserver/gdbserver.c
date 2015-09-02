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

#include "target_al.h"

#include "gdb_helpers.h"
#include "breakpoint.h"
#include "error.h"
#include "mem_log.h"
#include "special_chars.h"
#include "crc32.h"

#define GDBSERVER_KEEP_CHARS //for debugging: whether to keep track of chars received
#define GDBSERVER_KEEP_CHARS_NUM 128 //for debugging: number of chars to keep track of
#define GDBSERVER_LOG_PACKETS

//note: keep <256 or change PacketSize response in gdbserver
#define GDBSERVER_MAX_PACKET_LEN_RX 256
#define GDBSERVER_MAX_PACKET_LEN_TX 256

#define GDBSERVER_MAX_BLOCK_ACCESS 256
#define GDBSERVER_NUM_BKPT 256
#define GDBSERVER_POLL_INTERVAL 100

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
        "CC3200 Wi-Fi Debugger v0.1\n"\
        "Copyright 2015, MindTribe inc.\n"\
        "------------------------------------------\n"\
        "\n"\
        "Note: this is a work-in-progress.\n"\
        "Please see KNOWN_BUGS before using.\n"\
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
    char last_sent_packet[GDBSERVER_MAX_PACKET_LEN_TX];
    char cur_packet[GDBSERVER_MAX_PACKET_LEN_RX];
    char cur_checksum[2];
    int cur_packet_index;
    enum stop_reason stop_reason;
    struct target_al_interface *target;
    struct breakpoint breakpoints[GDBSERVER_NUM_BKPT];
    uint8_t halted;
    uint8_t gave_info;
    struct fileio_state_t fileio_state;
};

//disable GCC warning - braces bug 53119 in GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
struct gdbserver_state_t gdbserver_state = {
    .initialized = 0,
    .packet_phase = PACKET_NONE,
    .awaiting_ack = 0,
    .last_sent_packet = {0},
    .cur_packet = {0},
    .cur_checksum = {0},
    .cur_packet_index = 0,
    .stop_reason = STOPREASON_UNKNOWN,
    .gdb_connected = 0,
    .breakpoints = {0},
    .halted = 0,
    .gave_info = 0,
    .fileio_state = {0}
};
#pragma GCC diagnostic pop

static int isHex(char c)
{
    if((c>='0') && (c<='9')) return 1;
    if((c>='A') && (c<='F')) return 1;
    if((c>='a') && (c<='f')) return 1;
    return 0;
}

int gdbserver_init(void (*pPutChar)(char), void (*pGetChar)(char*), int (*pGetCharsAvail)(void), struct target_al_interface *target)
{
    if(gdb_helpers_init(pPutChar, pGetChar, pGetCharsAvail) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    gdbserver_state.target = target;

    if((*target->target_init)() == RET_FAILURE){
        RETURN_ERROR(ERROR_UNKNOWN);
    }
    if((*target->target_halt)() == RET_FAILURE){
        RETURN_ERROR(ERROR_UNKNOWN);
    }

    if((*target->target_poll_halted)(&gdbserver_state.halted) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(!gdbserver_state.halted) {RETURN_ERROR(ERROR_UNKNOWN);}

    //TODO: report the halted state to GDB

    gdbserver_state.gave_info = 0;
    gdbserver_state.fileio_state.fileio_waiting = 0;
    gdbserver_state.initialized = 1;
    return RET_SUCCESS;
}

void gdbserver_TransmitPacket(char* packet_data)
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

#ifdef GDBSERVER_LOG_PACKETS
    //log the packet
    mem_log_add(packet_data, 1);
#endif

    return;
}

void gdbserver_TransmitDebugMsgPacket(char* packet_data)
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

#ifdef GDBSERVER_LOG_PACKETS
    //log the packet
    mem_log_add(packet_data, 1);
#endif

    return;
}

void gdbserver_Interrupt(uint8_t signal)
{
    if((*gdbserver_state.target->target_halt)() == RET_FAILURE){
        gdbserver_TransmitPacket("OError halting target!");
        error_add(__FILE__, __LINE__, ERROR_UNKNOWN);
    }
    if((*gdbserver_state.target->target_poll_halted)(&gdbserver_state.halted) == RET_FAILURE){
        gdbserver_TransmitPacket("OError halting target!");
        error_add(__FILE__, __LINE__, ERROR_UNKNOWN);
    }
    if(!gdbserver_state.halted){
        gdbserver_TransmitPacket("OError halting target!");
        error_add(__FILE__, __LINE__, ERROR_UNKNOWN);
    }
    gdbserver_state.stop_reason = STOPREASON_INTERRUPT;
    char reply[4];
    reply[0] = 'S';
    sprintf(&(reply[1]), "%02X", signal);
    reply[3] = 0;
    gdbserver_TransmitPacket(reply);
    return;
}

int gdbserver_processChar(void)
{
    char c;
    gdb_helpers_GetChar(&c);

#ifdef GDBSERVER_KEEP_CHARS
    lastChars[curChar++ % GDBSERVER_KEEP_CHARS_NUM] = c;
    if(curChar%GDBSERVER_KEEP_CHARS_NUM) lastChars[curChar%GDBSERVER_KEEP_CHARS_NUM]  = 0; //zero-terminate
#endif

    if(gdbserver_state.packet_phase == PACKET_DATA){ //in data phase
        if(c == '$') {gdbserver_reset_error(__LINE__, c);}
        else if(c == '#'){
            gdbserver_state.packet_phase = PACKET_CHECKSUM; //next state
            gdbserver_state.cur_packet[gdbserver_state.cur_packet_index] = 0; //zero-terminate current packet
            gdbserver_state.cur_packet_index = 0;
        }
        else if(gdbserver_state.cur_packet_index >= (GDBSERVER_MAX_PACKET_LEN_RX - 1)) {gdbserver_reset_error(__LINE__, ERROR_UNKNOWN);} //too long
        else {gdbserver_state.cur_packet[gdbserver_state.cur_packet_index++] = c; }
    }
    else{ //all other cases
        switch(c){
        case '-':  //NACK (retransmit request)
            if(!gdbserver_state.awaiting_ack) { break; } //don't throw an error since this will start an infinite loop with GDB acking all the time
            gdbserver_TransmitPacket(gdbserver_state.last_sent_packet);
            break;
        case '+': //ACK
            if(!gdbserver_state.awaiting_ack) { break; } //don't throw an error since this will start an infinite loop with GDB acking all the time
            gdbserver_state.awaiting_ack = 0; //confirm successful send
            break;
        case '$':
            if(gdbserver_state.packet_phase != PACKET_NONE) { gdbserver_reset_error(__LINE__, ERROR_UNKNOWN); break; } //invalid character in packet
            gdbserver_state.packet_phase = PACKET_DATA;
            gdbserver_state.cur_packet_index = 0;
            break;
        case CHAR_END_TEXT: //CTRL-C in GDB
            gdbserver_Interrupt(SIGINT);
            if((*gdbserver_state.target->target_poll_halted)(&gdbserver_state.halted) == RET_FAILURE) { error_add(__FILE__, __LINE__, ERROR_UNKNOWN); }
            if(!gdbserver_state.halted) { error_add(__FILE__, __LINE__, ERROR_UNKNOWN); }
            break;
        default:
            switch(gdbserver_state.packet_phase){
            case PACKET_NONE:
                //do nothing - invalid character (junk)
                //gdbserver_reset_error(__LINE__, c); break; //invalid character at this point.
                break;
            case PACKET_CHECKSUM:
                if(!isHex(c)) { gdbserver_reset_error(__LINE__, c); break; } //checksum is hexadecimal chars only
                if(gdbserver_state.cur_packet_index == 0) {
                    gdbserver_state.cur_checksum[0] = c;
                    gdbserver_state.cur_packet_index++;
                }
                else{
                    gdbserver_state.cur_checksum[1] = c;
                    gdbserver_state.packet_phase = PACKET_NONE; //finished
                    if(gdbserver_processPacket() == RET_FAILURE) gdbserver_reset_error(__LINE__, ERROR_UNKNOWN); //process the last received packet.
                }
                break;
            default:
                gdbserver_reset_error(__LINE__, ERROR_UNKNOWN); //should never reach this point
            }
            break;
        }
    }

    return RET_SUCCESS;
}

void gdbserver_reset_error(int line, int error_code)
{
    error_add(__FILE__, line, error_code);
    gdbserver_state.packet_phase = PACKET_NONE;
    gdbserver_state.cur_packet_index = 0;
    gdbserver_state.awaiting_ack = 0;
}

enum gdbserver_query_name gdbserver_getGeneralQueryName(char* query)
{
    char queryName[20];
    int qi;
    for(qi=0; qi<19 && query[qi]!=':'; qi++){
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

int gdbserver_processGeneralQuery(char* queryString)
{
    enum gdbserver_query_name qname = gdbserver_getGeneralQueryName(queryString);
    if(qname == QUERY_ERROR){
        error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
        gdbserver_TransmitPacket("");
    }

    char response[50];

    switch(qname){
    case QUERY_SUPPORTED: //ask whether certain packet types are supported
        sprintf(response, "PacketSize=%04X", GDBSERVER_MAX_PACKET_LEN_RX);
        gdbserver_TransmitPacket(response);
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
        if(gdbserver_doMemCRC(&(queryString[4])) == RET_FAILURE) { error_add(__FILE__,__LINE__,ERROR_UNKNOWN); }
        break;
    default:
        gdbserver_TransmitPacket(""); //GDB reads this as "unsupported packet"
        break;//unsupported query as of now
    }

    return RET_SUCCESS;
}

void gdbserver_sendInfo(void)
{
    gdbserver_TransmitDebugMsgPacket(WFD_NAME_STRING);

    return;
}

int gdbserver_processPacket(void)
{
    //check the checksum
    unsigned int checksum;
    sscanf(gdbserver_state.cur_checksum, "%02X", &checksum);
    if((uint8_t)checksum != gdb_helpers_getChecksum(gdbserver_state.cur_packet)){
        gdbserver_reset_error(__LINE__, (uint8_t)strtol(gdbserver_state.cur_checksum, NULL, 16));
        gdb_helpers_Nack();
        return RET_SUCCESS;
    }

    //acknowledge
    gdb_helpers_Ack();

#ifdef GDBSERVER_LOG_PACKETS
    //log the packet
    mem_log_add(gdbserver_state.cur_packet, 0);
#endif

    //process the packet based on header character
    switch(gdbserver_state.cur_packet[0]){
    case 'q': //general query
        if(gdbserver_processGeneralQuery(&(gdbserver_state.cur_packet[1])) == RET_FAILURE) error_add(__FILE__, __LINE__, ERROR_UNKNOWN);
        break;
    case 'H': //set operation type and thread ID
        if((gdbserver_state.cur_packet[1] != 'c' && gdbserver_state.cur_packet[1] != 'g') //operation types
                || (gdbserver_state.cur_packet[2] != '0' && (gdbserver_state.cur_packet[2] != '-' || gdbserver_state.cur_packet[3] != '1'))){ //thread 0 or -1 only
            error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
            gdbserver_TransmitPacket("E1"); //reply error
            break;
        }
        gdbserver_TransmitPacket("OK"); //reply OK
        break;
    case '?': //ask for the reason why execution has stopped
        gdbserver_TransmitStopReason();
        break;
    case 'g': //request the register status
        if((*gdbserver_state.target->target_get_gdb_reg_string)(gdbserver_state.last_sent_packet)
                == RET_FAILURE){
            error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
            gdbserver_TransmitPacket("E0");
        }
        else{ gdbserver_TransmitPacket(gdbserver_state.last_sent_packet); } //send the register string
        break;
    case 'G': //write registers
        if((*gdbserver_state.target->target_put_gdb_reg_string)(&(gdbserver_state.cur_packet[1]))
                == RET_FAILURE){
            error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
            gdbserver_TransmitPacket("E0");
        }
        else {gdbserver_TransmitPacket("OK");}
        break;
    case 'm': //read memory
        if(gdbserver_readMemory(&(gdbserver_state.cur_packet[1])) == RET_FAILURE){
            gdbserver_TransmitPacket("E00");
            error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
        }
        break;
    case 'M': //write memory
        if(gdbserver_writeMemory(&(gdbserver_state.cur_packet[1])) == RET_FAILURE){
            gdbserver_TransmitPacket("E00");
            error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
        }
        break;
    case 'c': //continue
        if(strlen(gdbserver_state.cur_packet) != 1){
            //there is an address to go to. unsupported
            error_add(__FILE__,__LINE__,0);
            gdbserver_TransmitPacket("E0");
        }
        if(gdbserver_continue() == RET_FAILURE){
            error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
        }
        break;
    case 's': //single step
        if(strlen(gdbserver_state.cur_packet) != 1){
            //there is an address to go to. unsupported
            error_add(__FILE__,__LINE__,0);
            gdbserver_TransmitPacket("E0");
        }
        if((*gdbserver_state.target->target_step)()
                == RET_FAILURE){
            error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
            gdbserver_TransmitPacket("E0");
        }
        gdbserver_state.halted = 1; //still halted
        char reply[4];
        sprintf(reply, "S%02X", SIGTRAP);
        gdbserver_TransmitPacket(reply);
        break;
    case 'F': //reply to File I/O
        if(!gdbserver_state.fileio_state.fileio_waiting) error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
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
                    gdbserver_state.cur_packet[i+1] == 0){
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
                if((*gdbserver_state.target->target_write_register)(0, (uint32_t)return_code) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
            }
            else{ //number of bytes read
                if((*gdbserver_state.target->target_write_register)(
                        0, gdbserver_state.fileio_state.count - return_code) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
            }
            break;
        default:
            //TODO: reply
            return RET_SUCCESS;
            break;
        }
        //increment the PC to pass the BKPT instruction for continuing.
        uint32_t pc;
        if((*gdbserver_state.target->target_get_pc)(&pc) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        if((*gdbserver_state.target->target_set_pc)(pc+2) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

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


        break;
        default:
            gdbserver_TransmitPacket(""); //GDB reads this as "unsupported packet"
            //error_add(__FILE__,__LINE__, gdbserver_state.cur_packet[0]);
            break;
    }

    if(!gdbserver_state.gdb_connected){
        gdbserver_state.gdb_connected = 1;
    }

    return RET_SUCCESS;
}

int gdbserver_readMemory(char* argstring)
{
    //First, get the arguments
    char* addrstring;
    char* lenstring;
    for(int i=0; i<30; i++){
        if(argstring[i] == ','){
            argstring[i] = 0;
            addrstring = argstring;
            lenstring = &(argstring[i+1]);
            break;
        }
        if(i>=29) {RETURN_ERROR(ERROR_UNKNOWN);} //no comma found
    }

    uint32_t addr = strtol(addrstring, NULL, 16);
    uint32_t len = strtol(lenstring, NULL, 16);
    uint8_t data[GDBSERVER_MAX_BLOCK_ACCESS];

    if(len>GDBSERVER_MAX_BLOCK_ACCESS) {RETURN_ERROR(ERROR_UNKNOWN);}

    if((*gdbserver_state.target->target_mem_block_read)(addr, len, data) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    //construct the answer string
    char retstring[GDBSERVER_MAX_PACKET_LEN_TX];
    uint32_t i;
    for(i=0; i<len; i++){
        sprintf(&(retstring[i*2]), "%02X", data[i]);
    }
    retstring[i*2] = 0; //terminate

    gdbserver_TransmitPacket(retstring);

    return RET_SUCCESS;
}

//TODO: this function is untested (and so far unneccessary: GDB seems to be able to do
//SW breakpoints by using just memory access functions.
int gdbserver_setSWBreakpoint(uint32_t addr, uint8_t len_bytes)
{
    //find a free breakpoint location
    struct breakpoint *bkpt = 0;
    for(int bkpt_index=0; bkpt_index<GDBSERVER_NUM_BKPT; bkpt_index++){
        if(!gdbserver_state.breakpoints[bkpt_index].valid){
            bkpt = &(gdbserver_state.breakpoints[bkpt_index]);
            break;
        }
    }
    if(bkpt == 0){ //none found
        return RET_FAILURE;
    }

    bkpt->addr = addr;
    bkpt->len_bytes = len_bytes;
    bkpt->type = BKPT_SOFTWARE;

    //get the original instruction
    uint32_t inst;
    if((*gdbserver_state.target->target_mem_block_read)(addr, len_bytes, (uint8_t*)(&inst)) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    bkpt->ori_instr = inst;

    //place the breakpoint
    if((*gdbserver_state.target->target_set_sw_bkpt)(addr, len_bytes) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    bkpt->active = 1;
    bkpt->valid = 1;
    return RET_SUCCESS;
}

int gdbserver_doMemCRC(char* argstring)
{
    //First, get the arguments
    char* addrstring;
    char* lenstring;
    for(int i=0; i<30; i++){
        if(argstring[i] == ','){
            argstring[i] = 0;
            addrstring = argstring;
            lenstring = &(argstring[i+1]);
            break;
        }
        if(i>=29) {RETURN_ERROR(ERROR_UNKNOWN);} //no comma found
    }

    uint32_t addr = strtol(addrstring, NULL, 16);
    uint32_t len = strtol(lenstring, NULL, 16);
    uint32_t bytes_left = len;

    unsigned long long crc = 0xFFFFFFFF;

    uint8_t data[GDBSERVER_MAX_BLOCK_ACCESS];
    while(bytes_left){
        if((*gdbserver_state.target->target_mem_block_read)(addr,
                (GDBSERVER_MAX_BLOCK_ACCESS < bytes_left) ? GDBSERVER_MAX_BLOCK_ACCESS : bytes_left,
                        data)
                        == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);} //get some bytes
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

int gdbserver_writeMemory(char* argstring)
{
    //First, get the arguments
    char* addrstring;
    char* lenstring;
    char* datastring;
    for(int i=0; i<30; i++){
        if(argstring[i] == ','){
            argstring[i] = 0;
            addrstring = argstring;
            lenstring = &(argstring[i+1]);
            break;
        }
        if(i>=29) {RETURN_ERROR(ERROR_UNKNOWN);} //no comma found
    }
    for(int i=0; i<30; i++){
        if(lenstring[i] == ':'){
            lenstring[i] = 0;
            datastring = &(lenstring[i+1]);
            break;
        }
        if(i>=29) {RETURN_ERROR(ERROR_UNKNOWN);} //no colon found
    }

    uint32_t addr = strtol(addrstring, NULL, 16);
    uint32_t len = strtol(lenstring, NULL, 16);
    uint8_t data[GDBSERVER_MAX_BLOCK_ACCESS];

    if(len>GDBSERVER_MAX_BLOCK_ACCESS) RETURN_ERROR(len);

    //convert data
    for(uint32_t i=0; i<len; i++){
        unsigned int datai;
        sscanf(&(datastring[i*2]), "%02X", &datai);
        data[i] = (uint8_t) datai;
    }

    if((*gdbserver_state.target->target_mem_block_write)(addr, len, data) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    gdbserver_TransmitPacket("OK");

    return RET_SUCCESS;
}

void gdbserver_loop_task(void* params)
{
    while(1){
        if(!gdbserver_state.halted) gdbserver_pollTarget();
        if(gdb_helpers_CharsAvaliable()){
            gdbserver_processChar();
            while(gdbserver_state.packet_phase != PACKET_NONE){
                gdbserver_processChar();
            }
        }
    };

    (void)params; //avoid unused warning
    return;
}

int gdbserver_pollTarget(void)
{
    int old_halted = gdbserver_state.halted;
    if((*gdbserver_state.target->target_poll_halted)(&gdbserver_state.halted) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(old_halted && !gdbserver_state.halted) {RETURN_ERROR(ERROR_UNKNOWN);}
    if(!old_halted && gdbserver_state.halted){
        if(gdbserver_handleHalt() == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        char reply[4];
        switch(gdbserver_state.stop_reason){
        case STOPREASON_SEMIHOSTING:
            //semihosting is special: we don't report a stop to GDB, but instead
            //perform the semihosting action.
            if(gdbserver_handleSemiHosting() == RET_FAILURE){
                error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
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
            error_add(__FILE__, __LINE__, ERROR_UNKNOWN);
            break;
        }
    }
    return RET_SUCCESS;
}

int gdbserver_handleHalt(void)
{
    return (*gdbserver_state.target->target_handleHalt)(&gdbserver_state.stop_reason);
}

void gdbserver_TransmitStopReason(void){
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

int gdbserver_TransmitFileIOWrite(uint8_t descriptor, char *buf, uint32_t count)
{
    //we must wait for the previous file I/O to finish
    if(gdbserver_state.fileio_state.fileio_waiting) {RETURN_ERROR(ERROR_UNKNOWN);}

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

int gdbserver_TransmitFileIORead(uint8_t descriptor, char *buf, uint32_t count)
{
    //we must wait for the previous file I/O to finish
    if(gdbserver_state.fileio_state.fileio_waiting) {RETURN_ERROR(ERROR_UNKNOWN);}

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

int gdbserver_continue(void)
{
    if(!gdbserver_state.halted) {RETURN_ERROR(ERROR_UNKNOWN);}

    if(!gdbserver_state.gave_info){
        gdbserver_sendInfo();
        gdbserver_state.gave_info = 1;
    }

    if((*gdbserver_state.target->target_continue)()
            == RET_FAILURE){
        gdbserver_TransmitPacket("E0");
        RETURN_ERROR(ERROR_UNKNOWN);
    }
    gdbserver_state.halted = 0; //force halted off so polling starts

    return RET_SUCCESS;
}

int gdbserver_handleSemiHosting(void)
{
    //a semihosting operation is supposed to happen. Find out which one
    if((*gdbserver_state.target->target_querySemiHostOp)(
            &(gdbserver_state.fileio_state.last_semihost_op)) == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}

    switch(gdbserver_state.fileio_state.last_semihost_op.opcode){
    case SEMIHOST_WRITECONSOLE:
        if(gdbserver_TransmitFileIOWrite(1,
                (char*)gdbserver_state.fileio_state.last_semihost_op.param1,
                gdbserver_state.fileio_state.last_semihost_op.param2)
                == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        break;
    case SEMIHOST_READCONSOLE:
        if(gdbserver_TransmitFileIORead(
                gdbserver_state.fileio_state.last_semihost_op.param1,
                (char*)gdbserver_state.fileio_state.last_semihost_op.param2,
                gdbserver_state.fileio_state.last_semihost_op.param3)
                == RET_FAILURE) {RETURN_ERROR(ERROR_UNKNOWN);}
        break;
    default:
        //TODO: reply
        return RET_SUCCESS;
        break;
    }

    return RET_SUCCESS;
}
