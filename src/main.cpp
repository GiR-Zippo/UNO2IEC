#include "global_defines.h"
#include "log.h"
#include "iec_driver.h"
#include "interface.h"

#include "SD.h"
#include "dosCommands.hpp"

// The global IEC handling singleton:
static IEC iec(8);
static DOS dos(iec);

void setup()
{
    digitalWrite(13, HIGH);
    // Initialize serial and wait for port to open:
    Serial.begin(DEFAULT_BAUD_RATE);
    Serial.setTimeout(SERIAL_TIMEOUT_MSECS);

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
