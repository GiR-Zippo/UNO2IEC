#include "DOS.hpp"
#include "iec_driver.h"
#include "cbmdefines.h"
#include "Log.hpp"

const char errorString[100][25] PROGMEM =
{
    {"00,OK,00,00"},
    {"01,FILES SCRATCHED"},
    {0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},
    {0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},
    {"20,READ ERROR"},
    {"21,READ ERROR"},
    {"22,READ ERROR"},
    {0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},
    {0x00},{"31, SYNTAX ERROR"},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00}, {0x00}, //39
    {0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00}, {0x00}, //49
    {0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00}, {0x00}, //59
    {0x00},{"61, FILE NOT OPEN"},{0x00},{0x00},{0x00},{"65, NO BLOCK"},{0x00},{0x00},{0x00}, {0x00}, //69
    {"70, NO CHANNEL"},{0x00},{0x00},{"73,PIRANHA DOS V0.1 1541"},{0x00},{0x00},{0x00},{0x00},{0x00}, {0x00}, //79
    {0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00}, {0x00}, //89
    {0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00},{0x00}, {0x00}
};

DOS::DOS(IEC& iec): _iec(iec),
_cmd(*reinterpret_cast<IEC::ATNCmd*>(&_DataBuffer[sizeof(_DataBuffer) / 2]))
{
    _directory = "/";
}

DOS::~DOS()
{
}

void DOS::DriveReset(bool soft)
{
    _init = true;
    for (byte i=0; i < 16; i++)
    {
        _channels[i].atn = ATN_IDLE;
        _channels[i].open = false;
        if (_channels[i].cmd != NULL)
            delete _channels[i].cmd;
        _channels[i].cmd = NULL;
    }
    _filemode = FMODE_FAT;

    if (soft)
        return;
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

void DOS::changeDriveNumber(byte channel)
{
    byte n = sprintf(_DataBuffer, "%s", _channels[channel].cmd->str);
    _iec.SetDeviceNumber(_DataBuffer[3]);
}

void DOS::changeDirectory(byte channel)
{
    byte n = sprintf(_DataBuffer, "%s", _channels[channel].cmd->str);
    for (uint16_t i = 1; i != n; i++)
    {
        if (_DataBuffer[i] == 0 || _DataBuffer[i] == ' ')
            break;
        _directory += _DataBuffer[i];
    }
    prepareSendListing();
    finishSendListing();
}

void DOS::changeDirectoryUp(byte channel)
{
    for (int i = _directory.length(); i != 1; i--)
    {
        if (_directory.charAt(i) == '/')
        {
            _directory.setCharAt(i, ' ');
            _directory.trim();
            break;
        }
        _directory.setCharAt(i, ' ');
    }
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
    if (channel == CBM::CMD_CHANNEL && _channels[channel].atn == ATN_CMD)
    {
        if (_init)
        {
            _init = false;
            sendStatus(CBM::ErrIntro);
            return;
        }
    }

    //The others
    _channels[channel].atn == ATN_IDLE;
    sendStatus(CBM::ErrOK);
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

/*************************************************\
|*                  IEC Functions                *|
\*************************************************/

///- Send DriveStatus
void DOS::sendStatus(CBM::IOErrorMessage msg)
{
    byte i;
    noInterrupts();

    for(i = 0; i < sizeof(errorString[msg]) ; ++i)
    {
        char a = pgm_read_byte_near(errorString[msg] + i);
        _iec.send(a);
    }
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
void DOS::sendListingLine(byte len, word& basicPtr)
{
    byte i;

    // Increment next line pointer
    // note: minus two here because the line number is included in the array already.
    basicPtr += len + 3;

    // Send that pointer
    _iec.send(basicPtr bitand 0xFF);
    _iec.send(basicPtr >> 8);

    // Send line contents
    for(i = 0; i < len; i++)
        _iec.send(_DataBuffer[i]);

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