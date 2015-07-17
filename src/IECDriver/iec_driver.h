#ifndef IEC_DRIVER_H
#define IEC_DRIVER_H

#include <Arduino.h>
#include "global_defines.h"
#include "cbmdefines.h"
#include "Protocol.hpp"

// IEC protocol timing consts:
#define TIMING_BIT          70  // bit clock hi/lo time     (us)
#define TIMING_NO_EOI       1   // delay before bits        (us)
#define TIMING_EOI_WAIT     200 // delay to signal EOI      (us)
#define TIMING_EOI_THRESH   20  // threshold for EOI detect (*10 us approx)
#define TIMING_STABLE_WAIT  1  // line stabilization       (us)
#define TIMING_ATN_PREDELAY 50  // delay required in atn    (us)
#define TIMING_ATN_DELAY    100 // delay required after atn (us)
#define TIMING_FNF_DELAY    100 // delay after fnf?         (us)

// Version 0.5 equivalent timings: 70, 5, 200, 20, 20, 50, 100, 100

// TIMING TESTING:
//
// The consts: 70,20,200,20,20,50,100,100 has been tested without debug print
// to work stable on my (Larsp)'s DTV at 700000 < F_CPU < 9000000
// using a 32 MB MMC card
//

// The IEC bus pin configuration on the Arduino side
// See timeoutWait below.
#define TIMEOUT  65000

class IEC
{
public:

    typedef struct _tagATNCMD
    {
        byte code;
        byte str[ATN_CMD_MAX_LENGTH];
        byte strLen;
    } ATNCmd;

    IEC(byte deviceNumber = DEFAULT_IEC_DEVICE);
    ~IEC() {}

    // Initialise iec driver
    //
    boolean Init();

    // Checks if CBM is sending an attention message. If this is the case,
    // the message is recieved and stored in atn_cmd.
    //
    ATNCheck checkATN(ATNCmd& cmd);

    // Checks if CBM is sending a reset (setting the RESET line high). This is typicall
    // when the CBM is reset itself. In this case, we are supposed to reset all states to initial.
    boolean checkRESET();

    // Sends a byte. The communication must be in the correct state: a load command
    // must just have been recieved. If something is not OK, FALSE is returned.
    //
    boolean send(byte data);

    // Same as IEC_send, but indicating that this is the last byte.
    //
    boolean sendEOI(byte data);

    // A special send command that informs file not found condition
    //
    boolean sendFNF();

    // Recieves a byte
    //
    byte receive();

    byte deviceNumber() const;
    void SetDeviceNumber(const byte deviceNumber);

    void setDriveMode(FloppyMode mode);
    FloppyMode GetDriveMode() { return (FloppyMode)_driveMode; }

    void SetPins(byte atn, byte clock, byte data, byte srqIn, byte reset);
    IECState state() const;

    void testINPUTS();
private:
    byte timeoutWait(byte waitBit, boolean whileHigh);

    ///-1541
    byte receiveByte1541(void);
    boolean sendByte1541(byte data, boolean signalEOI);

    ///-1571
    boolean sendByte1571(byte data, boolean signalEOI);

    boolean turnAround(void);
    boolean undoTurnAround(void);

    // false = LOW, true == HIGH
    inline boolean readPIN(byte pinNumber)
    {
        // To be able to read line we must be set to input, not driving.
        pinMode(pinNumber, INPUT);
        return digitalRead(pinNumber) ? true : false;
    }

    inline boolean readATN()
    {
        return readPIN(m_atnPin);
    }

    inline boolean readDATA()
    {
        return readPIN(m_dataPin);
    }

    inline boolean readCLOCK()
    {
        return readPIN(m_clockPin);
    }

    inline boolean readRESET()
    {
        return !readPIN(m_resetPin);
    }

    inline boolean readSRQIN()
    {
        return readPIN(m_srqInPin);
    }

    // true == PULL == HIGH, false == RELEASE == LOW
    inline void writePIN(byte pinNumber, boolean state)
    {
        pinMode(pinNumber, state ? OUTPUT : INPUT);
        digitalWrite(pinNumber, state ? LOW : HIGH);
    }

    inline void writeATN(boolean state)
    {
        writePIN(m_atnPin, state);
    }

    inline void writeDATA(boolean state)
    {
        writePIN(m_dataPin, state);
    }

    inline void writeCLOCK(boolean state)
    {
        writePIN(m_clockPin, state);
    }

    // Drive Mode
    byte _driveMode;
    // communication must be reset
    byte m_state;
    byte m_deviceNumber;

    byte m_atnPin;
    byte m_dataPin;
    byte m_clockPin;
    byte m_srqInPin;
    byte m_resetPin;
};

#endif
