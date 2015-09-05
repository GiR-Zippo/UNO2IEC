#ifndef __DOS_HPP__
#define __DOS_HPP__

#include "iec_driver.h"
#include "SdSpi.h"
#include "SdFat.h"

struct Channel
{
    bool open;
    byte atn;
    IEC::ATNCmd* cmd;
};

enum FileMode
{
    FMODE_FAT = 0,
    FMODE_D64 = 1,
    FMODE_MAX = 2
};
static const char * EnumStrings[100];

class Driver;
class File;

class DOS
{
    public:
        DOS(IEC& iec);
        ~DOS();

        void DriveReset(bool soft = false);

        ///- DOSHandler.cpp
        byte Update(void);

        ///- DOSChannel.cpp
        bool OpenChannel(byte channel, IEC::ATNCmd& cmd, byte atn);
        bool CloseChannel(byte channel, byte atn);
        void ChannelTalk(byte channel);
        void ChannelListen(byte channel);
        void ChannelCommand(byte channel, IEC::ATNCmd& cmd, byte atn);

        bool Command(byte channel, ATNCommand req);
    private:
        void Load(byte channel);
        void Save(byte channel);
        void getDirectory(byte channel);
        void changeDriveNumber(byte channel);
        void changeDirectory(byte channel);
        void changeDirectoryUp(byte channel);
        void getStatus(byte channel);
        bool selectImage(byte channel);
        String _directory;
        
    ///- FatDriver
    private:
        void FAT_GetDir();
        void FAT_Load(unsigned char* filename);
        void FAT_Save(unsigned char* filename);
        
        void ConvertFilename(unsigned char* filename);
        
    ///- D64 Driver
    private:
        void D64_GetDir();

    ///- IEC Functions
    private:
        void sendStatus(CBM::IOErrorMessage msg);
        void prepareSendListing();
        void sendListingLine(byte len, word &basicPtr);
        void finishSendListing();

        char *ToChar(unsigned char* args) { return reinterpret_cast<char *>(args); }
    ///- IEC LowLevel
    private:
        SdFat SD;
        IEC::ATNCmd            &_cmd;
        IEC                    &_iec;
        Channel                 _channels[16];
        bool                    _init;
        char                    _DataBuffer[256];  ///- Databuffer for I/O
        FileMode                _filemode;
};
#endif
 
