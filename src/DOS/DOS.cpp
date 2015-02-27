#include "DOS.hpp"
#include "iec_driver.h"
#include "cbmdefines.h"
#include "Log.hpp"

DOS::DOS(IEC& iec): _iec(iec),
_cmd(*reinterpret_cast<IEC::ATNCmd*>(&_DataBuffer[sizeof(_DataBuffer) / 2]))
{
    EnumStrings[CBM::ErrOK]                     = "00,OK,00,00";
    EnumStrings[CBM::ErrFilesScratched]         = "01,FILES SCRATCHED";
    EnumStrings[CBM::ErrBlockHeaderNotFound]    = "20,READ ERROR";
    EnumStrings[CBM::ErrSyncCharNotFound]       = "21,READ ERROR";
    EnumStrings[CBM::ErrDataBlockNotFound]      = "22,READ ERROR";

    EnumStrings[CBM::ErrIntro]                  = "73,PIRANHA DOS V0.1 1541";
    _directory = "/";
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
#ifdef DEBUG
    Serial.print("Initializing SD card...");
#endif
    pinMode(53, OUTPUT);
    if (!SD.begin(53))
    {
#ifdef DEBUG
        Serial.println("initialization failed!");
#endif
        return;
    }
#ifdef DEBUG
    Serial.println("initialization done.");
#endif
    _filemode = FMODE_D64;
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
        case FMODE_D64:
            D64_GetDir();
            break;
        default:
            break;
    };
    finishSendListing();
}

void DOS::changeDirectory(byte channel)
{
    _directory ="";
    byte n = sprintf(_DataBuffer, "%s", _channels[channel].cmd->str);
    for (uint16_t i = 1; i != n; i++)
        _directory += _DataBuffer[i];
    prepareSendListing();
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

void DOS::Load(byte channel)
{
    switch (_filemode)
    {
        case FMODE_FAT:
            FAT_Load(_channels[channel].cmd->str);
            break;
        default:
            break;
    }
}

void DOS::Save(byte channel)
{
    switch (_filemode)
    {
        case FMODE_FAT:
            FAT_Save(_channels[channel].cmd->str);
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