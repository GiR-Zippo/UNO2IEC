#include "DOS.hpp"
#include "Log.hpp"

bool DOS::OpenChannel(byte channel, IEC::ATNCmd& cmd, byte atn)
{
    digitalWrite(13, HIGH);
    _channels[channel].atn = atn;
    _channels[channel].open = true;
    _channels[channel].cmd = new IEC::ATNCmd(cmd);
    return true;
}

bool DOS::CloseChannel(byte channel, byte atn)
{
    digitalWrite(13, LOW);
    _channels[channel].atn = IEC::ATN_IDLE;
    _channels[channel].open = false;
    delete _channels[channel].cmd;
    _channels[channel].cmd = NULL;
    return true;
}

void DOS::ChannelTalk(byte channel)
{
#ifdef DEBUG
    Serial.print("TALK: ");
    sprintf_P(_DataBuffer, (PGM_P)F("cmd: %s (len: %d) "), _channels[channel].cmd->str,_channels[channel].cmd->strLen);
    Log(Information, FAC_IFACE, _DataBuffer);
#endif

    if (_channels[channel].cmd == NULL)
        getStatus(channel);
    //Send Directory
    else if (_channels[channel].cmd->str[0] == '$' && _channels[channel].cmd->strLen == 1)
        getDirectory(channel);
    else if (_channels[channel].cmd->str[0] == '$' &&
             _channels[channel].cmd->str[1] == '/' && 
             _channels[channel].cmd->strLen > 1)
        changeDirectory(channel);
    //Select an Image
    else if (_channels[channel].cmd->str[0] == '$' &&
             _channels[channel].cmd->str[1] == ':' && 
             _channels[channel].cmd->strLen > 1)
        selectImage(channel);
    else if (_channels[channel].cmd->str[0] == 'U' &&
             _channels[channel].cmd->str[1] == '0' && 
             _channels[channel].cmd->strLen > 1)
        return;
    else
        Load(channel);
    return;
}

void DOS::ChannelListen(byte channel)
{
#ifdef DEBUG
    Serial.print("LISTEN: ");
    sprintf_P(_DataBuffer, (PGM_P)F("cmd: %s (len: %d) "), _channels[channel].cmd->str,_channels[channel].cmd->strLen);
    Log(Information, FAC_IFACE, _DataBuffer);
#endif

    if (_channels[channel].cmd == NULL)
        getStatus(channel);
    else
        Save(channel);
    return;
}
