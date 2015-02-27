#include "DOS.hpp"
#include "D64Driver.hpp"
#include <Arduino.h>
#include "SdFat.h"

void DOS::D64_GetDir()
{
    Serial.println("D64");
    File D64_CurrentFile = SD.open("1.D64");

    if (!D64_CurrentFile.seek(D64Driver::SeekToDiskName()))
        return;

 /*   Serial.println("OK");
    String buffer;
    D64_CurrentFile.read(buffer, 23);
    
    for (int i=0; i <= 23; i++)
        if (buffer[i] == 0xA0)
            buffer[i] = ' ';

    word basicPtr = C64_BASIC_START;
    byte n = sprintf(_DataBuffer, "%c%c\x12\x22%s", 0, 0, buffer.c_str());
    sendListingLine(n, _DataBuffer, basicPtr);*/
    D64_CurrentFile.close();
}

uint32_t D64Driver::SeekToDiskName()
{
    return SeekTrackSector(18, 1) + 144;
}

uint32_t D64Driver::SeekTrackSector(byte track, byte sector)
{
    uint32_t count = 0;
    for (byte t=1; t < track; t++)
    {
        for (byte s=0; s < sectorsPerTrack[t] + (sector-1); s++)
            count += 256;
    }
    return count;
}
