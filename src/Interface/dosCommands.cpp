#include "dosCommands.hpp"
#include "iec_driver.h"
#include "cbmdefines.h"
#include "interface.h"
#include "log.h"

#include "SD.h"

DOS::DOS(IEC& iec): _iec(iec),
_cmd(*reinterpret_cast<IEC::ATNCmd*>(&_DataBuffer[sizeof(_DataBuffer) / 2]))
{
    EnumStrings[CBM::ErrOK]                     = "00,OK,00,00";
    EnumStrings[CBM::ErrFilesScratched]         = "01,FILES SCRATCHED";
    EnumStrings[CBM::ErrBlockHeaderNotFound]    = "20,READ ERROR";
    EnumStrings[CBM::ErrSyncCharNotFound]       = "21,READ ERROR";
    EnumStrings[CBM::ErrDataBlockNotFound]      = "22,READ ERROR";

    EnumStrings[CBM::ErrIntro]                  = "73,CBM DOS V2.6 1541";
}

DOS::~DOS()
{
}

void DOS::DriveReset()
{
    _init = true;
    for (byte i=0; i < 16; i++)
    {
        _channels[i].atn = IEC::ATN_IDLE;
        _channels[i].open = false;
        if (_channels[i].cmd != NULL)
            delete _channels[i].cmd;
        _channels[i].cmd = NULL;
    }

    Serial.print("Initializing SD card...");
    pinMode(53, OUTPUT);
    if (!SD.begin(53))
    {
        Serial.println("initialization failed!");
        return;
    }
    Serial.println("initialization done.");

    _filemode = FMODE_FAT;
}

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
                if(retATN == IEC::ATN_CMD)
                    CloseChannel(chan, retATN);
                break;
            case IEC::ATN_CODE_DATA:
                if(retATN == IEC::ATN_CMD)
                    SetATN(chan, retATN);
                else if(retATN == IEC::ATN_CMD_TALK)
                    ChannelTalk(chan);
                break;
        };
    } // IEC not idle

    return retATN;
} // handler

///- Open DriveChannel
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
    Serial.println("TALK");
    sprintf_P(_DataBuffer, (PGM_P)F("cmd: %s (len: %d) "), _channels[channel].cmd->str,_channels[channel].cmd->strLen);
    Log(Information, FAC_IFACE, _DataBuffer);

    if (_channels[channel].cmd == NULL)
        getStatus(channel);
    //Ŝend Directory
    else if (_channels[channel].cmd->str[0] == '$' && _channels[channel].cmd->strLen == 1)
        getDirectory(channel);
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
        SendFile(channel);
    return;
}

void DOS::SetATN(byte channel, byte atn)
{
    _channels[channel].atn = atn;
}

///-privater Kram
void DOS::getDirectory(byte channel)
{
    prepareSendListing();
    switch (_filemode)
    {
        case FMODE_FAT:
            FAT_GetDir();
            break;
        default:
            break;
    };
    finishSendListing();
}

bool DOS::selectImage(byte channel)
{
    prepareSendListing();
    finishSendListing();
}

void DOS::getStatus(byte channel)
{
    //Channel 15
    if (channel == CBM::CMD_CHANNEL && _channels[channel].atn == IEC::ATN_CMD)
    {
        if (_init)
        {
            _init = false;
            sendStatus(getIOErrorMessageString(CBM::ErrIntro));
            return;
        }
    }

    //The others
    _channels[channel].atn == IEC::ATN_IDLE;
    sendStatus(getIOErrorMessageString(CBM::ErrOK));
    return;
}

void DOS::SendFile(byte channel)
{
    switch (_filemode)
    {
        case FMODE_FAT:
            FAT_SendFile(_channels[channel].cmd->str);
            break;
        default:
            break;
    }
}

const char* DOS::getIOErrorMessageString(int enumVal)
{
    return EnumStrings[enumVal];
}

/*************************************************\
|*                  IEC Functions                *|
\*************************************************/

///- Send DriveStatus
void DOS::sendStatus(String data)
{
    byte i;
    noInterrupts();
    for(i = 0; i < data.length() ; ++i)
        _iec.send(data[i]);
    _iec.sendEOI('\r'); //send CR
    interrupts();
}

///- Prepare for sending listing
void DOS::prepareSendListing()
{
    noInterrupts();
    _iec.send(C64_BASIC_START bitand 0xff);
    _iec.send((C64_BASIC_START >> 8) bitand 0xff);
    interrupts();
}

///- send listing line
void DOS::sendListingLine(byte len, const char* text, word& basicPtr)
{
    byte i;

    // Increment next line pointer
    // note: minus two here because the line number is included in the array already.
    basicPtr += len + 5 - 2;

    // Send that pointer
    _iec.send(basicPtr bitand 0xFF);
    _iec.send(basicPtr >> 8);

    // Send line contents
    for(i = 0; i < len; i++)
        _iec.send(text[i]);

    // Finish line
    _iec.send(0);
}

///- finish em
void DOS::finishSendListing()
{
    noInterrupts();
    _iec.send(0);
    _iec.sendEOI(0);
    interrupts();
}

/*void DOS::sendFile()
{
    // Send file bytes, such that the last one is sent with EOI.
    byte resp;
    Serial.write('S'); // ask for file size.
    byte len = Serial.readBytes(serCmdIOBuf, 3);
    // it is supposed to answer with S<highByte><LowByte>
    if(3 not_eq len or serCmdIOBuf[0] not_eq 'S')
        return; // got some garbage response.
    word bytesDone = 0, totalSize = (((word)((byte)serCmdIOBuf[1])) << 8) bitor (byte)(serCmdIOBuf[2]);

    bool success = true;
    // Initial request for a bunch of bytes, here we specify the read size for every subsequent 'R' command.
    // This begins the transfer "game".
    Serial.write('N');                                                                                  // ask for a byte/bunch of bytes
    Serial.write(MAX_BYTES_PER_REQUEST);                // specify the arduino serial library buffer limit for best performance / throughput.
    do {
        len = Serial.readBytes(serCmdIOBuf, 2); // read the ack type ('B' or 'E')
        if(2 not_eq len) {
            strcpy_P(serCmdIOBuf, (PGM_P)F("2 Host bytes expected, stopping"));
            Log(Error, FAC_IFACE, serCmdIOBuf);
            success = false;
            break;
        }
        resp = serCmdIOBuf[0];
        len = serCmdIOBuf[1];
        if('B' == resp or 'E' == resp) {
            byte actual = Serial.readBytes(serCmdIOBuf, len);
            if(actual not_eq len) {
                strcpy_P(serCmdIOBuf, (PGM_P)F("Host bytes expected, stopping"));
                success = false;
                Log(Error, FAC_IFACE, serCmdIOBuf);
                break;
            }
#ifdef EXPERIMENTAL_SPEED_FIX
            if('E' not_eq resp) // if not received the final buffer, initiate a new buffer request while we're feeding the CBM.
                Serial.write('R'); // ask for a byte/bunch of bytes
#endif
            // so we get some bytes, send them to CBM.
            for(byte i = 0; success and i < len; ++i) { // End if sending to CBM fails.
#ifndef EXPERIMENTAL_SPEED_FIX
                noInterrupts();
#endif
                if(resp == 'E' and i == len - 1)
                    success = m_iec.sendEOI(serCmdIOBuf[i]); // indicate end of file.
                else
                    success = m_iec.send(serCmdIOBuf[i]);
#ifndef EXPERIMENTAL_SPEED_FIX
                interrupts();
#endif
                ++bytesDone;

            }
#ifndef EXPERIMENTAL_SPEED_FIX
            if('E' not_eq resp) // if not received the final buffer, initiate a new buffer request while we're feeding the CBM.
                Serial.write('R'); // ask for a byte/bunch of bytes
#endif
        }
        else {
            strcpy_P(serCmdIOBuf, (PGM_P)F("Got unexp. cmd resp.char."));
            Log(Error, FAC_IFACE, serCmdIOBuf);
            success = false;
        }
    } while(resp == 'B' and success); // keep asking for more as long as we don't get the 'E' or something else (indicating out of sync).
    // If something failed and we have serial bytes in recieve queue we need to flush it out.
    if(not success and Serial.available())
    {
        while(Serial.available())
            Serial.read();
    }
    
    if(success)
    {
        sprintf_P(serCmdIOBuf, (PGM_P)F("Transferred %u of %u bytes."), bytesDone, totalSize);
        Log(Success, FAC_IFACE, serCmdIOBuf);
    }
}*/