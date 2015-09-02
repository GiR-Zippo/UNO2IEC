#include "DOS.hpp"
#include "Protocol.hpp"
#include <string.h>

/*
DIFATN chan:15 code:240 cmd: I (len: 1) retATN: 1
DIFATN chan:13 code:240 cmd: # (len: 1) retATN: 1
DIFATN chan:15 code:96 cmd: U1:13 0 01 00 (len: 13) retATN: 1
DIFATN chan:13 code:96 cmd:  (len: 0) retATN: 3
TALK: DIFcmd: # (len: 1) 
FILE cmd: # (len: 1) DIFATN chan:13 code:224 cmd:  (len: 0) retATN: 1
DIFATN chan:15 code:96 cmd: UI (len: 2) retATN: 1
*/

bool DOS::Command(byte channel, ATNCommand req)
{
    if (req == ATN_CODE_TALK)
    {
        if (_channels[channel].cmd == NULL)
            getStatus(channel);
        else if (_channels[channel].cmd->str[0] == '$' &&
                 _channels[channel].cmd->strLen == 1)
            getDirectory(channel);
        else if (_channels[channel].cmd->str[0] == '$' &&
                 _channels[channel].cmd->str[1] == '/' &&
                 _channels[channel].cmd->strLen > 1)
            changeDirectory(channel);
        else if (_channels[channel].cmd->str[0] == '$' &&
                 _channels[channel].cmd->str[1] == '.' &&
                 _channels[channel].cmd->str[2] == '.' &&
                 _channels[channel].cmd->strLen == 3)
            changeDirectoryUp(channel);
        //Select an Image
        else if (_channels[channel].cmd->str[0] == '$' &&
                 _channels[channel].cmd->str[1] == ':' &&
                 _channels[channel].cmd->strLen > 1)
            selectImage(channel);
        else
            return false;
    }
    else if (req == ATN_CODE_OPEN || req == ATN_CODE_DATA || req == ATN_CODE_TALK)
    {
        if ( _channels[channel].cmd->str[0] == 'U' &&
             _channels[channel].cmd->str[1] == '0' &&
             _channels[channel].cmd->str[2] == '>' &&
             _channels[channel].cmd->strLen > 2)
            changeDriveNumber(channel);
        else
            return false;
    }
    else
        return false;
    return true;
}