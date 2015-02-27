#include "DOS.hpp"
#include "Log.hpp"

///- Ganz wichtiger Imperiumskram
byte DOS::Update(void)
{
    if(_iec.checkRESET())
    {
        // IEC reset line is in reset state, so we should set all states in reset.
        DriveReset();
        return IEC::ATN_RESET;
    }
    noInterrupts();
    IEC::ATNCheck retATN = _iec.checkATN(_cmd);
    interrupts();

    if(retATN == IEC::ATN_ERROR)
    {
        strcpy_P(_DataBuffer, (PGM_P)F("ATNCMD: IEC_ERROR!"));
        Log(Error, FAC_IFACE, _DataBuffer);
        DriveReset();
    }
    // Did anything happen from the host side?
    else if(retATN not_eq IEC::ATN_IDLE)
    {
        // A command is recieved, make cmd string null terminated
        _cmd.str[_cmd.strLen] = '\0';
        byte chan = _cmd.code bitand 0x0F;
        byte code = _cmd.code bitand 0xF0;
        {
            sprintf_P(_DataBuffer, (PGM_P)F("ATN chan:%d code:%d cmd: %s (len: %d) retATN: %d"), chan, code, _cmd.str, _cmd.strLen, retATN);
            Log(Information, FAC_IFACE, _DataBuffer);
        }

        switch (code)
        {
            case IEC::ATN_CODE_OPEN:
                OpenChannel(chan, _cmd, retATN);
                break;
            case IEC::ATN_CODE_CLOSE:
                CloseChannel(chan, retATN);
                break;
            case IEC::ATN_CODE_DATA:
                if(retATN == IEC::ATN_CMD)
                    SetATN(chan, retATN);
                else if(retATN == IEC::ATN_CMD_TALK)
                    ChannelTalk(chan);
                else if(retATN == IEC::ATN_CMD_LISTEN)
                    ChannelListen(chan);
                break;
        };
    } // IEC not idle

    return retATN;
} // handler 
