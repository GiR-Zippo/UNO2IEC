#include "DOS.hpp"
#include "Log.hpp"

bool DOS::OpenChannel(byte channel, IEC::ATNCmd& cmd, byte atn)
{
    digitalWrite(13, HIGH);
    _channels[channel].atn = atn;
    _channels[channel].open = true;
    _channels[channel].cmd = new IEC::ATNCmd(cmd);
    Command(channel, ATN_CODE_OPEN);
    return true;
}

bool DOS::CloseChannel(byte channel, byte atn)
{
    digitalWrite(13, LOW);
    _channels[channel].atn = ATN_IDLE;
    _channels[channel].open = false;
    delete _channels[channel].cmd;
    _channels[channel].cmd = NULL;
    return true;
}

void DOS::ChannelCommand(byte channel, IEC::ATNCmd& cmd, byte atn)
{
    _channels[channel].atn = atn;
    if (cmd.strLen > 0)
    {
        _channels[channel].cmd = new IEC::ATNCmd(cmd);
        Command(channel, ATN_CODE_DATA);
    }
}

void DOS::ChannelTalk(byte channel)
{
#ifdef DEBUG
    Serial.print("TALK: ");
    sprintf_P(_DataBuffer, (PGM_P)F("cmd: %s (len: %d) "), _channels[channel].cmd->str,_channels[channel].cmd->strLen);
    Log(Information, FAC_IFACE, _DataBuffer);
#endif
    if (!Command(channel, ATN_CODE_TALK))
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
