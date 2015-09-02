#include "DOS.hpp"
#include <Arduino.h>

#define toupper(c) ((c) >= 'a' && (c) <= 'z') ? (c) & ~0x20 : (c)

void DOS::FAT_GetDir()
{
    word basicPtr = C64_BASIC_START;
    File root = SD.open(_directory.c_str());
    root.rewindDirectory();
    byte n = sprintf(_DataBuffer, "%c%c\x12\x22%s%-22s\x22", 0, 0, "SDCARD:", _directory.c_str());
    sendListingLine(n, basicPtr);
    while(true)
    {
        File entry = root.openNextFile();
        if (! entry)
            break;

        char name[25];
        entry.getName(name, sizeof(name));
        for(int i=0; name[i]; i++)
            name[i] = toupper(name[i]);
        
        if (!entry.isDirectory())
        {
            byte n = sprintf(_DataBuffer, "%c%c   \x22%-24s\x22PRG", 0, 0, name);
            sendListingLine(n, basicPtr);
        }
        else
        {
            byte n = sprintf(_DataBuffer, "%c%c   \x22$/%-22s\x22<DIR>", 0, 0, name);
            sendListingLine(n, basicPtr);
        }
        entry.close();
    }
}

void DOS::FAT_Load(unsigned char* filename)
{
    //TODO accept joker
#ifdef DEBUG    
    Serial.print("FILE ");
    Serial.print(_DataBuffer);
#endif
    ConvertFilename(filename);
    if (SD.exists(_DataBuffer))
    {
#ifdef DEBUG 
        Serial.print(" EXISTS");
#endif
        File FAT_CurrentFile = SD.open(_DataBuffer);
        if (FAT_CurrentFile)
        {
#ifdef DEBUG 
            Serial.print(" SIZE ");
            Serial.println(FAT_CurrentFile.size());
#endif
            while (FAT_CurrentFile.available())
            {
                byte n = FAT_CurrentFile.read(_DataBuffer, 255);
                n--;//Der ist 1 zu groß
#ifdef DEBUG 
                Serial.print("#");
#endif
                noInterrupts();
                if (n != 254)
                    n--;

                for(byte i = 0; i <= n; i++)
                    if (!_iec.send(_DataBuffer[i]))
                        break;
                if (n != 254)
                    if(!_iec.sendEOI(_DataBuffer[++n]))
                        break;
                interrupts();
            }
            FAT_CurrentFile.close();
        }
        else
        {
#ifdef DEBUG 
            Serial.println(" NOT EXISTS.");
#endif
            _iec.sendFNF();
        }
    }
}

void DOS::FAT_Save(unsigned char* filename)
{
    ConvertFilename(filename);
    File FAT_CurrentFile = SD.open(_DataBuffer, FILE_WRITE);
    bool done = false;
    do
    {
        byte bytesInBuffer = 0;
        do
        {
            noInterrupts();
            FAT_CurrentFile.write(_iec.receive());
            interrupts();
            done = (_iec.state() bitand eoiFlag) or (_iec.state() bitand errorFlag);
        } while(bytesInBuffer < sizeof(_DataBuffer) and not done);
    } while(not done);
    FAT_CurrentFile.close();
}

void DOS::ConvertFilename(unsigned char* filename)
{
    sprintf(_DataBuffer, "%s/%s", _directory.c_str(), filename);
}