/*  ------------------------------------------------------------
    Copyright(C) 2015 MindTribe Product Engineering, Inc.
    All Rights Reserved.

    Author:     Sander Vocke, MindTribe Product Engineering
                <sander@mindtribe.com>

    Target(s):  ISO/IEC 9899:1999 (target independent)
    --------------------------------------------------------- */

#include "gdbserver.h"
#include "target_al.h"
#include "gdb_helpers.h"
#include "common.h"
#include "error.h"
#include "mem_log.h"

#ifdef GDBSERVER_KEEP_CHARS
char lastChars[GDBSERVER_KEEP_CHARS_NUM];
unsigned int curChar = 0;
#endif

enum gdbserver_packet_phase{
    PACKET_NONE = 0,
    PACKET_DATA,
    PACKET_CHECKSUM
};

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

enum gdbserver_stop_reason{
    STOPREASON_UNKNOWN = 0
};

struct gdbserver_state_t{
    int initialized;
    enum gdbserver_packet_phase packet_phase;
    int awaiting_ack;
    char last_sent_packet[MAX_GDB_PACKET_LEN];
    char cur_packet[MAX_GDB_PACKET_LEN];
    char cur_checksum[2];
    int cur_packet_index;
    enum gdbserver_stop_reason stop_reason;
    struct target_al_interface *target;
};
struct gdbserver_state_t gdbserver_state = {
    .initialized = 0,
    .packet_phase = PACKET_NONE,
    .awaiting_ack = 0,
    .last_sent_packet = {0},
    .cur_packet = {0},
    .cur_checksum = {0},
    .cur_packet_index = 0,
    .stop_reason = STOPREASON_UNKNOWN
};

int gdbserver_init(void (*pPutChar)(char), void (*pGetChar)(char*), struct target_al_interface *target)
{
    if(gdb_helpers_init((void*)pPutChar, (void*)pGetChar) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);
    gdbserver_state.target = target;

    if((*target->target_init)() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    if((*target->target_halt)() == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    gdbserver_state.initialized = 1;
    return RET_SUCCESS;
}

void gdbserver_TransmitPacket(char* packet_data)
{
    gdb_helpers_TransmitPacket(packet_data);
    gdbserver_state.awaiting_ack = 1;

#ifdef GDBSERVER_LOG_PACKETS
    //log the packet
    mem_log_add(packet_data, 1);
#endif

    return;
}

int gdbserver_processChar(void)
{
    char c;
    gdb_helpers_GetChar(&c);

#ifdef GDBSERVER_KEEP_CHARS
    lastChars[curChar++ % GDBSERVER_KEEP_CHARS_NUM] = c;
#endif

    if(gdbserver_state.packet_phase == PACKET_DATA){ //in data phase
        if(c == '$') {gdbserver_reset_error(__LINE__, c);}
        else if(c == '#'){
            gdbserver_state.packet_phase = PACKET_CHECKSUM; //next state
            gdbserver_state.cur_packet[gdbserver_state.cur_packet_index] = 0; //zero-terminate current packet
            gdbserver_state.cur_packet_index = 0;
        }
        else if(gdbserver_state.cur_packet_index >= (MAX_GDB_PACKET_LEN - 1)) {gdbserver_reset_error(__LINE__, ERROR_UNKNOWN);} //too long
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
        default:
            switch(gdbserver_state.packet_phase){
            case PACKET_NONE:
                gdbserver_reset_error(__LINE__, c); break; //invalid character at this point.
                break;
            case PACKET_CHECKSUM:
                if(!gdb_helpers_isHex(c)) { gdbserver_reset_error(__LINE__, c); break; } //checksum is hexadecimal chars only
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
        if(wfd_stringsEqual(queryName, (char*) gdbserver_query_strings[i])){
            return (enum gdbserver_query_name) i; //found it
        }
    }

    return QUERY_ERROR; //wasn't able to decode
}

int gdbserver_processGeneralQuery(char* queryString)
{
    enum gdbserver_query_name qname = gdbserver_getGeneralQueryName(queryString);
    if(qname == QUERY_ERROR) RETURN_ERROR(ERROR_UNKNOWN);

    switch(qname){
    case QUERY_SUPPORTED: //ask whether certain packet types are supported
        //for now, respond as if we don't know of the existence of any features at all.
        gdbserver_TransmitPacket("");
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
        //this is bare-metal debugging, so we reply "all threads".
        gdbserver_TransmitPacket("QC-1");
        break;
    case QUERY_ATTACHED: //ask whether the debugger attached to an existing process, or created a new one.
        //default to "attached" (this informs GDB that some stuff was already going on)
        gdbserver_TransmitPacket("1");
        break;
    default:
        gdbserver_TransmitPacket(""); //GDB reads this as "unsupported packet"
        break;//unsupported query as of now
    }

    return RET_SUCCESS;
}

int gdbserver_processPacket(void)
{
    //check the checksum
    if(gdb_helpers_hexToByte(gdbserver_state.cur_checksum) != gdb_helpers_getChecksum(gdbserver_state.cur_packet)){
        gdbserver_reset_error(__LINE__, gdb_helpers_hexToByte(gdbserver_state.cur_checksum));
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
    char *temp;
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
        (*gdbserver_state.target->target_get_gdb_reg_string)(&temp); //get the register string
        gdbserver_TransmitPacket(temp); //send the register string
        break;
    case 'G': //write registers
        if((*gdbserver_state.target->target_put_gdb_reg_string)(&(gdbserver_state.cur_packet[1]))
                == RET_FAILURE){
            error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
            gdbserver_TransmitPacket("E0");
        }
        else gdbserver_TransmitPacket("OK");
        break;
    case 'm': //read memory
        if(gdbserver_reportMemory(&(gdbserver_state.cur_packet[1])) == RET_FAILURE){
            error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
        }
        break;
    case 'M': //write memory
        if(gdbserver_writeMemory(&(gdbserver_state.cur_packet[1])) == RET_FAILURE){
            error_add(__FILE__,__LINE__,ERROR_UNKNOWN);
        }
        break;
    default:
        gdbserver_TransmitPacket(""); //GDB reads this as "unsupported packet"
        //error_add(__FILE__,__LINE__, gdbserver_state.cur_packet[0]);
        break;
    }

    return RET_SUCCESS;
}

int gdbserver_reportMemory(char* argstring)
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
        if(i>=29) RETURN_ERROR(ERROR_UNKNOWN); //no comma found
    }

    uint32_t addr = gdb_helpers_hexToInt(addrstring);
    uint32_t len = gdb_helpers_hexToInt(lenstring);
    uint32_t data = 0;

    //TODO: for now we only support word accesses, fix this!
    if(len!=4) RETURN_ERROR(ERROR_UNKNOWN);

    if((*gdbserver_state.target->target_mem_read)(addr, &data) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    //construct the answer string
    char retstring[9];
    retstring[8]=0;
    gdb_helpers_byteToHex((uint8_t)((data>>0)&0xFF), &(retstring[0]));
    gdb_helpers_byteToHex((uint8_t)((data>>8)&0xFF), &(retstring[2]));
    gdb_helpers_byteToHex((uint8_t)((data>>16)&0xFF), &(retstring[4]));
    gdb_helpers_byteToHex((uint8_t)((data>>24)&0xFF), &(retstring[6]));

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
        if(i>=29) RETURN_ERROR(ERROR_UNKNOWN); //no comma found
    }
    for(int i=0; i<30; i++){
        if(lenstring[i] == ':'){
            lenstring[i] = 0;
            datastring = &(lenstring[i+1]);
            break;
        }
        if(i>=29) RETURN_ERROR(ERROR_UNKNOWN); //no colon found
    }

    uint32_t addr = gdb_helpers_hexToInt(addrstring);
    uint32_t len = gdb_helpers_hexToInt(lenstring);
    uint32_t data = gdb_helpers_hexToInt_LE(datastring);

    //TODO: for now we only support word accesses, fix this!
    if(len!=4) RETURN_ERROR(ERROR_UNKNOWN);

    if((*gdbserver_state.target->target_mem_write)(addr, data) == RET_FAILURE) RETURN_ERROR(ERROR_UNKNOWN);

    gdbserver_TransmitPacket("OK");

    return RET_SUCCESS;
}

int gdbserver_loop(void)
{
    while(1){
        gdbserver_processChar();
    };

    return RET_SUCCESS;
}

void gdbserver_TransmitStopReason(void){
    switch(gdbserver_state.stop_reason){
    default:
    case STOPREASON_UNKNOWN:
        gdbserver_TransmitPacket("T00");
        break;
    }
}
