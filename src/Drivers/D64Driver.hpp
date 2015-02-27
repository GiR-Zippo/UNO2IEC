#ifndef __D64DRIVER_HPP__
#define __D64DRIVER_HPP__
#include <stdint.h>

byte sectorsPerTrack[41] = {0, 
                21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,21,
                19,19,19,19,19,19,19,
                18,18,18,18,18,18,
                17,17,17,17,17,17,17,17,17,17
};

class D64Driver
{
    public:
        static uint32_t SeekToDiskName();
        static uint32_t SeekTrackSector(byte track, byte sector);
};
#endif