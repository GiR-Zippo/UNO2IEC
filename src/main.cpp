#define DIGITALIO_NO_MIX_ANALOGWRITE
#include "digitalIOPerformance.h"

#include "global_defines.h"
#include "Log.hpp"
#include "iec_driver.h"
#include "DOS.hpp"

// The global IEC handling singleton:
static IEC iec(8);
static DOS dos(iec);

void setup()
{
    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);
#ifdef DEBUG
    Serial.begin(DEFAULT_BAUD_RATE);
    Serial.setTimeout(SERIAL_TIMEOUT_MSECS);
#endif
    // initial connection handling.
    registerFacilities();

    iec.SetDeviceNumber(8);
    iec.SetPins(ATN, CLK, DATA, SRQ, RES);

    // set all digital pins in a defined state.
    iec.Init();
    dos.DriveReset();
} // setup


void loop()
{
    if(ATN_RESET == dos.Update())
        while(ATN_RESET == dos.Update());
} // loop
