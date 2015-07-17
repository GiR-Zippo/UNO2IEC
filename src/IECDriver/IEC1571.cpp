#include "iec_driver.h"

boolean IEC::sendByte1571(byte data, boolean signalEOI)
{
    // Listener must have accepted previous data
    if(timeoutWait(m_clockPin, false))
        return false;

    // Say we're ready
    writeCLOCK(true);

    // Wait for listener to be ready
    if(timeoutWait(m_clockPin, false))
        return false;

    if(signalEOI)
    {
        // FIXME: Make this like sd2iec and may not need a fixed delay here.

        // Signal eoi by waiting 200 us
        delayMicroseconds(TIMING_EOI_WAIT);

        // get eoi acknowledge:
        if(timeoutWait(m_dataPin, true))
            return false;

        if(timeoutWait(m_dataPin, false))
            return false;
    }

    delayMicroseconds(TIMING_NO_EOI);

    // Send bits
    for(byte n = 0; n < 8; n++)
    {
        // FIXME: Here check whether data pin goes low, if so end (enter cleanup)!

        writeCLOCK(false);
        if(timeoutWait(m_atnPin, true))
            return false;
        writeDATA((data bitand 1) ? false : true);


        data >>= 1;
    }

    writeCLOCK(true);
    writeDATA(false);

    // FIXME: Maybe make the following ending more like sd2iec instead.

    // Line stabilization delay
    delayMicroseconds(TIMING_STABLE_WAIT);

    // Wait for listener to accept data
    if(timeoutWait(m_dataPin, true))
        return false;

    return true;
} // sendByte 
