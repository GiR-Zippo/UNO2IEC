#include "iec_driver.h"

byte IEC::receiveByte1541(void)
{
    m_state = noFlags;

    // Wait for talker ready
    if(timeoutWait(m_clockPin, false))
        return 0;

    // Say we're ready
    writeDATA(false);

    // Record how long CLOCK is high, more than 200 us means EOI
    byte n = 0;
    while(readCLOCK() and (n < 20))
    {
        delayMicroseconds(10);  // this loop should cycle in about 10 us...
        n++;
    }

    if(n >= TIMING_EOI_THRESH)
    {
        // EOI intermission
        m_state or_eq eoiFlag;

        // Acknowledge by pull down data more than 60 us
        writeDATA(true);
        delayMicroseconds(TIMING_BIT);
        writeDATA(false);

        // but still wait for clk
        if(timeoutWait(m_clockPin, true))
            return 0;
    }

    // Sample ATN
    if(false == readATN())
        m_state or_eq atnFlag;

    byte data = 0;
    // Get the bits, sampling on clock rising edge:
    for(n = 0; n < 8; n++)
    {
        data >>= 1;
        if(timeoutWait(m_clockPin, false))
            return 0;
        data or_eq (readDATA() ? (1 << 7) : 0);
        if(timeoutWait(m_clockPin, true))
            return 0;
    }

    // Signal we accepted data:
    writeDATA(true);

    return data;
} // receiveByte

// IEC Send byte standard function
//
// Sends the byte and can signal EOI
//
boolean IEC::sendByte1541(byte data, boolean signalEOI)
{
    // Listener must have accepted previous data
    if(timeoutWait(m_dataPin, true))
        return false;

    // Say we're ready
    writeCLOCK(false);

    // Wait for listener to be ready
    if(timeoutWait(m_dataPin, false))
        return false;

    if(signalEOI)
    {
        // FIXME: Make this like sd2iec and may not need a fixed delay here.

        // Signal eoi by waiting 200 us
        //delayMicroseconds(TIMING_EOI_WAIT);

        // get eoi acknowledge:
        if(timeoutWait(m_dataPin, true))
            return false;

        if(timeoutWait(m_dataPin, false))
            return false;
    }

    //delayMicroseconds(TIMING_NO_EOI);

    // Send bits
    for(byte n = 0; n < 8; n++)
    {
        // FIXME: Here check whether data pin goes low, if so end (enter cleanup)!
        writeCLOCK(true);
        // set data
        writeDATA((data bitand 1) ? false : true);

        delayMicroseconds(TIMING_BIT);
        writeCLOCK(false);
        delayMicroseconds(TIMING_BIT);

        data >>= 1;
    }

    writeCLOCK(true);
    writeDATA(false);

    // FIXME: Maybe make the following ending more like sd2iec instead.

    // Line stabilization delay
    //delayMicroseconds(TIMING_STABLE_WAIT);

    // Wait for listener to accept data
    if(timeoutWait(m_dataPin, true))
        return false;

    return true;
} // sendByte
