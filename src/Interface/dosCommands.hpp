#ifndef __DOSCOMMANDS_HPP__
#define __DOSCOMMANDS_HPP__
#include <Arduino.h>
#include "iec_driver.h"

struct Channel
{
    bool open;
    byte atn;
    IEC::ATNCmd* cmd;
};

enum FileMode
{
    FMODE_FAT = 0,
    FMODE_MAX = 1
};

static const char * EnumStrings[100];
class Driver;
class File;
class DOS
{
    public:
        DOS(IEC& iec);
        ~DOS();
        
        void DriveReset();
        byte Update(void);

        bool OpenChannel(byte channel, IEC::ATNCmd& cmd, byte atn);
        bool CloseChannel(byte channel, byte atn);
        void ChannelTalk(byte channel);

        void SetATN(byte channel, byte atn);
        
    private:
        void SendFile(byte channel);
        void getDirectory(byte channel);
        void getStatus(byte channel);
        bool selectImage(byte channel);
        const char * getIOErrorMessageString(int enumVal);
    
        
    ///- FatDriver
    private:
        void FAT_GetDir();
        void FAT_SendFile(unsigned char* filename);
    ///- D64 Driver
    private:
        
    ///- IEC Functions
    private:
        void sendStatus(String data);
        void prepareSendListing();
        void sendListingLine(byte len, const char* text, word &basicPtr);
        void finishSendListing();

    private:
        IEC::ATNCmd    &_cmd;
        IEC            &_iec;
        Channel         _channels[16];
        bool            _init;
        char            _DataBuffer[256];
        FileMode        _filemode;
};
#endif
