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

    iec.setDeviceNumber(8);
    iec.setPins(ATN, CLK, DATA, SRQ, RES);

    // set all digital pins in a defined state.
    iec.init();
    dos.DriveReset();
} // setup


void loop()
{
    if(IEC::ATN_RESET == dos.Update())
    {
        // Wait for it to get out of reset.
        while(IEC::ATN_RESET == dos.Update());
    }
} // loop
