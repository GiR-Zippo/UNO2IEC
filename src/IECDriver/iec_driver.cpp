#include "iec_driver.h"
#include "Log.hpp"
using namespace CBM;

/******************************************************************************
 *                                                                             *
 *                                TIMING SETUP                                 *
 *                                                                             *
 ******************************************************************************/

IEC::IEC(byte deviceNumber) :
    m_state(noFlags), m_deviceNumber(deviceNumber),
    m_atnPin(ATN), m_dataPin(DATA),
    m_clockPin(CLK), m_srqInPin(SRQ), m_resetPin(RES),
    _driveMode(FM_1541)
{
} // ctor

byte IEC::timeoutWait(byte waitBit, boolean whileHigh)
{
    word t = 0;
    boolean c;

    while(t < TIMEOUT)
    {
        // Check the waiting condition:
        c = readPIN(waitBit);

        if(whileHigh)
            c = not c;

        if(c)
            return false;

        delayMicroseconds(2); // The aim is to make the loop at least 3 us
        t++;
    }

    // If down here, we have had a timeout.
    // Release lines and go to inactive state with error flag
    writeCLOCK(false);
    writeDATA(false);

    m_state = errorFlag;

    // Wait for ATN release, problem might have occured during attention
    while(not readATN());

    // Note: The while above is without timeout. If ATN is held low forever,
    //       the CBM is out in the woods and needs a reset anyways.

    return true;
} // timeoutWait

// IEC turnaround
boolean IEC::turnAround(void)
{
    // Wait until clock is released
    if(timeoutWait(m_clockPin, false))
        return false;

    writeDATA(false);
    delayMicroseconds(TIMING_BIT);
    writeCLOCK(true);
    delayMicroseconds(TIMING_BIT);

    return true;
} // turnAround

// this routine will set the direction on the bus back to normal
// (the way it was when the computer was switched on)
boolean IEC::undoTurnAround(void)
{
    writeDATA(true);
    delayMicroseconds(TIMING_BIT);
    writeCLOCK(false);
    delayMicroseconds(TIMING_BIT);

    // wait until the computer releases the clock line
    if(timeoutWait(m_clockPin, true))
        return false;

    return true;
} // undoTurnAround

/******************************************************************************
 *                                                                             *
 *                               Public functions                              *
 *                                                                             *
 ******************************************************************************/

// This function checks and deals with atn signal commands
//
// If a command is recieved, the cmd-string is saved in cmd. Only commands
// for *this* device are dealt with.
//
// Return value, see IEC::ATNCheck definition.
ATNCheck IEC::checkATN(ATNCmd& cmd)
{
    ATNCheck ret = ATN_IDLE;
    byte i = 0;

    if(not readATN())
    {
        // Attention line is active, go to listener mode and get message. Being fast with the next two lines here is CRITICAL!
        writeDATA(true);
        writeCLOCK(false);
        delayMicroseconds(TIMING_ATN_PREDELAY);

        // Get first ATN byte, it is either LISTEN or TALK
        ATNCommand c = (ATNCommand)receive();
        if(m_state bitand errorFlag)
            return ATN_ERROR;

        if(c == (ATN_CODE_LISTEN bitor m_deviceNumber))
        {
            // Okay, we will listen.
            // Get the first cmd byte, the cmd code
            c = (ATNCommand)receive();
            if (m_state bitand errorFlag)
                return ATN_ERROR;

            cmd.code = c;

            // If the command is DATA and it is not to expect just a small command on the command channel, then
            // we're into something more heavy. Otherwise read it all out right here until UNLISTEN is received.
            if((c bitand 0xF0) == ATN_CODE_DATA and (c bitand 0xF) not_eq CMD_CHANNEL) {
                // A heapload of data might come now, too big for this context to handle so the caller handles this, we're done here.
                Log(Information, FAC_IEC, "LDATA");
                ret = ATN_CMD_LISTEN;
            }
            else if(c not_eq ATN_CODE_UNLISTEN)
            {
                // Some other command. Record the cmd string until UNLISTEN is sent
                for(;;) {
                    c = (ATNCommand)receive();
                    if(m_state bitand errorFlag)
                        return ATN_ERROR;

                    if((m_state bitand atnFlag) and (ATN_CODE_UNLISTEN == c))
                        break;

                    if(i >= ATN_CMD_MAX_LENGTH)
                    {
                        // Buffer is going to overflow, this is an error condition
                        // FIXME: here we should propagate the error type being overflow so that reading error channel can give right code out.
                        Log(Information, FAC_IEC, "LDATA TOO BIG");
                        return ATN_ERROR;
                    }
                    cmd.str[i++] = c;
                }
                ret = ATN_CMD;
            }
        }
        else if (c == (ATN_CODE_TALK bitor m_deviceNumber))
        {
            // Okay, we will talk soon, record cmd string while ATN is active
            // First byte is cmd code, that we CAN at least expect. All else depends on ATN.
            c = (ATNCommand)receive();
            if(m_state bitand errorFlag)
                return ATN_ERROR;
            cmd.code = c;

            while(not readATN()) {
                if(readCLOCK()) {
                    c = (ATNCommand)receive();
                    if(m_state bitand errorFlag)
                        return ATN_ERROR;

                    if(i >= ATN_CMD_MAX_LENGTH) {
                        // Buffer is going to overflow, this is an error condition
                        // FIXME: here we should propagate the error type being overflow so that reading error channel can give right code out.
                        return ATN_ERROR;
                    }
                    cmd.str[i++] = c;
                }
            }

            // Now ATN has just been released, do bus turnaround
            if(not turnAround())
                return ATN_ERROR;

            // We have recieved a CMD and we should talk now:
            ret = ATN_CMD_TALK;

        }
        else if (c == (ATN_CODE_DATA bitor m_deviceNumber))
        {
            Log(Information, FAC_IEC, "ATN_CODE_DATA");
        }
        else if (c == (ATN_CODE_CLOSE bitor m_deviceNumber))
        {
            Log(Information, FAC_IEC, "ATN_CODE_CLOSE");
        }
        else if (c == (ATN_CODE_OPEN bitor m_deviceNumber))
        {
            Log(Information, FAC_IEC, "ATN_CODE_OPEN");
        }
        else
        {
            // Either the message is not for us or insignificant, like unlisten.
            delayMicroseconds(TIMING_ATN_DELAY);
            writeDATA(false);
            writeCLOCK(false);

            // Wait for ATN to release and quit
            while(not readATN());
        }
    }
    else {
        // No ATN, keep lines in a released state.
        writeDATA(false);
        writeCLOCK(false);
    }

    // some delay is required before more ATN business can take place.
    delayMicroseconds(TIMING_ATN_DELAY);

    cmd.strLen = i;
    return ret;
} // checkATN


boolean IEC::checkRESET()
{
    return readRESET();
} // checkATN


// IEC_receive receives a byte
//
byte IEC::receive()
{
    switch (_driveMode)
    {
        case FM_1541:
            return receiveByte1541();
    }
} // receive


// IEC_send sends a byte
//
boolean IEC::send(byte data)
{
    switch (_driveMode)
    {
        case FM_1541:
            return sendByte1541(data, false);
    };
} // send


// Same as IEC_send, but indicating that this is the last byte.
//
boolean IEC::sendEOI(byte data)
{
    switch (_driveMode)
    {
        case FM_1541:
            if(sendByte1541(data, true))
            {
                // As we have just send last byte, turn bus back around
                if(undoTurnAround())
                    return true;
            }
            break;
    };
    return false;
} // sendEOI


// A special send command that informs file not found condition
//
boolean IEC::sendFNF()
{
    // Message file not found by just releasing lines
    writeDATA(false);
    writeCLOCK(false);

    // Hold back a little...
    delayMicroseconds(TIMING_FNF_DELAY);

    return true;
} // sendFNF


// Set all IEC_signal lines in the correct mode
//
boolean IEC::Init()
{
    // make sure the output states are initially LOW.
    pinMode(m_atnPin, OUTPUT);
    pinMode(m_dataPin, OUTPUT);
    pinMode(m_clockPin, OUTPUT);
    digitalWrite(m_atnPin, false);
    digitalWrite(m_dataPin, false);
    digitalWrite(m_clockPin, false);

#ifdef RESET_C64
    pinMode(m_resetPin, OUTPUT);
    digitalWrite(m_resetPin, false);        // only early C64's could be reset by a slave going high.
#endif

    // initial pin modes in GPIO.
    pinMode(m_atnPin, INPUT);
    pinMode(m_dataPin, INPUT);
    pinMode(m_clockPin, INPUT);
    pinMode(m_resetPin, INPUT);

    // Set port low, we don't need internal pullup
    // and DDR input such that we release all signals
    //  IEC_PORT and_eq compl(IEC_BIT_ATN bitor IEC_BIT_CLOCK bitor IEC_BIT_DATA);
    //  IEC_DDR and_eq compl(IEC_BIT_ATN bitor IEC_BIT_CLOCK bitor IEC_BIT_DATA);

    m_state = noFlags;
    return true;
} // init


#ifdef DEBUGLINES
void IEC::testINPUTS()
{
    unsigned long now = millis();
    // show states every second.
    //if(now - m_lastMillis >= 1000)
    {
        //m_lastMillis = now;
        char buffer[80];
        sprintf(buffer, "Lines, ATN: %s CLOCK: %s DATA: %s SRQ: %s",
                (readATN() ? "HIGH" : "LOW"), (readCLOCK() ? "HIGH" : "LOW"), (readDATA() ? "HIGH" : "LOW"), (readSRQIN() ? "HIGH" : "LOW"));
        Log(Information, FAC_IEC, buffer);
    }
} // testINPUTS

void IEC::testOUTPUTS()
{
    static bool lowOrHigh = false;
    unsigned long now = millis();
    // switch states every second.
    if(now - m_lastMillis >= 1000) {
        m_lastMillis = now;
        char buffer[80];
        sprintf(buffer, "Lines: CLOCK: %s DATA: %s", (lowOrHigh ? "HIGH" : "LOW"), (lowOrHigh ? "HIGH" : "LOW"));
        Log(Information, FAC_IEC, buffer);
        writeCLOCK(lowOrHigh);
        writeDATA(lowOrHigh);
        lowOrHigh xor_eq true;
    }
} // testOUTPUTS
#endif


byte IEC::deviceNumber() const
{
    return m_deviceNumber;
} // deviceNumber


void IEC::SetDeviceNumber(const byte deviceNumber)
{
    m_deviceNumber = deviceNumber;
} // setDeviceNumber


void IEC::SetPins(byte atn, byte clock, byte data, byte srqIn, byte reset)
{
    m_atnPin = atn;
    m_clockPin = clock;
    m_dataPin = data;
    m_resetPin = reset;
    m_srqInPin = srqIn;
} // setPins


IECState IEC::state() const
{
    return static_cast<IECState>(m_state);
} // state

