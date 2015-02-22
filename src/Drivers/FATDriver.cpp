#include "dosCommands.hpp"

#include <Arduino.h>
#include "SD.h"

void DOS::FAT_GetDir()
{
    File root = SD.open("/");
    while(true)
    {
        File entry = root.openNextFile();
        if (! entry)
            break;
        
        if (!entry.isDirectory())
        {
            word basicPtr = C64_BASIC_START;
            byte n = sprintf(_DataBuffer, "%c%c%s", 0, 0, entry.name());
            sendListingLine(n, _DataBuffer, basicPtr);

        }
        /*else
        {
            Serial.println("/");
            printDirectory(entry, numTabs+1);
        }*/
        entry.close();
    }
}

void DOS::FAT_SendFile(unsigned char* filename)
{
    Serial.print("FILE ");
    Serial.print(reinterpret_cast<char *>(filename));
    if (SD.exists(reinterpret_cast<char *>(filename)))
    {
        Serial.print(" EXISTS");
        File myFile = SD.open(reinterpret_cast<char *>(filename));
        if (myFile)
        {
            Serial.print(" SIZE ");
            Serial.println(myFile.size());

            while (myFile.available())
            {
                byte n = myFile.read(_DataBuffer, 255);
                Serial.print("#");

                noInterrupts();
                if (n != 255)
                    n -= 1;
                for(byte i = 0; i < n; i++)
                    _iec.send(_DataBuffer[i]);
                if (n != 255)
                    _iec.sendEOI(_DataBuffer[n+1]);
                interrupts();
            }
            Serial.println("#");
            myFile.close();
        }
        else
        {
            Serial.println(" NOT EXISTS.");
            _iec.sendFNF();
        }
    }
}

