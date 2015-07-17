#ifndef __PROTOCOL_HPP__
#define __PROTOCOL_HPP__

enum IECState
{
    noFlags   = 0,
    eoiFlag   = (1 << 0),   // might be set by Iec_receive
    atnFlag   = (1 << 1),   // might be set by Iec_receive
    errorFlag = (1 << 2)    // If this flag is set, something went wrong and
};

// Return values for checkATN:
enum ATNCheck
{
    ATN_IDLE = 0,       // Nothing recieved of our concern
    ATN_CMD = 1,        // A command is recieved
    ATN_CMD_LISTEN = 2, // A command is recieved and data is coming to us
    ATN_CMD_TALK = 3,   // A command is recieved and we must talk now
    ATN_ERROR = 4,      // A problem occoured, reset communication
    ATN_RESET = 5       // The IEC bus is in a reset state (RESET line).
};

// IEC ATN commands:
enum ATNCommand {
    ATN_CODE_LISTEN = 0x20,
    ATN_CODE_TALK = 0x40,
    ATN_CODE_DATA = 0x60,
    ATN_CODE_CLOSE = 0xE0,
    ATN_CODE_OPEN = 0xF0,
    ATN_CODE_UNLISTEN = 0x3F,
    ATN_CODE_UNTALK = 0x5F
};

// ATN command struct maximum command length:
#define ATN_CMD_MAX_LENGTH 40
// default device number listening unless explicitly stated in ctor:
#define DEFAULT_IEC_DEVICE 8

enum FloppyMode
{
    FM_1541 = 0,
    FM_1571 = 1
};
#endif
