#include "global_defines.h"
#include "log.h"
#include "iec_driver.h"
#include "interface.h"

// Pin 13 has a LED connected on most Arduino boards.
const byte ledPort = 13;
const byte numBlinks = 4;
const char connectionString[] PROGMEM = "connect_arduino:%u\r";
const char okString[] PROGMEM = "OK>";

static void waitForPeer();

// The global IEC handling singleton:
static IEC iec(8);
static Interface iface(iec);

static ulong lastMillis = 0;

void setup()
{
    // Initialize serial and wait for port to open:
    Serial.begin(DEFAULT_BAUD_RATE);
    Serial.setTimeout(SERIAL_TIMEOUT_MSECS);

    // Now we're ready to wait for the PI to respond to our connection attempts.
    // initial connection handling.
    waitForPeer();

    // set all digital pins in a defined state.
    iec.init();
    lastMillis = millis();
} // setup


void loop()
{
    if(IEC::ATN_RESET == iface.handler())
    {
        // Wait for it to get out of reset.
        while(IEC::ATN_RESET == iface.handler());
    }
} // loop


// Establish initial connection to the media host.
static void waitForPeer()
{
    char tempBuffer[80];
    unsigned deviceNumber, atnPin, clockPin, dataPin, resetPin, srqInPin,
             hour, minute, second, year, month, day;

    // initialize the digital LED pin as an output.
    pinMode(ledPort, OUTPUT);

    boolean connected = false;
    while(not connected) {
        // empty all avail. in buffer.
        while(Serial.available())
            Serial.read();
        sprintf_P(tempBuffer, connectionString, CURRENT_UNO2IEC_PROTOCOL_VERSION);
        //strcpy_P(tempBuffer, connectionString);
        Serial.print(tempBuffer);
        Serial.flush();
        // Indicate to user we are waiting for connection.
        for(byte i = 0; i < numBlinks; ++i) {
            digitalWrite(ledPort, HIGH);   // turn the LED on (HIGH is the voltage level)
            delay(500 / numBlinks / 2);               // wait for a second
            digitalWrite(ledPort, LOW);   // turn the LED on (HIGH is the voltage level)
            delay(500 / numBlinks / 2);               // wait for a second
        }
        strcpy_P(tempBuffer, okString);
        connected = Serial.find(tempBuffer);
    } // while(not connected)

    // Now read the whole configuration string from host, ends with CR. If we don't get THIS string, we're in a bad state.
    if(Serial.readBytesUntil('\r', tempBuffer, sizeof(tempBuffer)))
    {
        sscanf_P(tempBuffer, (PGM_P)F("%u|%u|%u|%u|%u|%u|%u-%u-%u.%u:%u:%u"),
                 &deviceNumber, &atnPin, &clockPin, &dataPin, &resetPin, &srqInPin,
                 &year, &month, &day, &hour, &minute, &second);

        // we got the config from the HOST.
        iec.setDeviceNumber(deviceNumber);
        iec.setPins(30, 31, 32, 33, 34);
        //iec.setPins(atnPin, clockPin, dataPin, srqInPin, resetPin);
        iface.setDateTime(year, month, day, hour, minute, second);
    }
    registerFacilities();

    // We're in business.
    sprintf_P(tempBuffer, (PGM_P)F("CONNECTED, READY FOR IEC DATA WITH CBM AS DEV %u."), deviceNumber);
    Log(Success, 'M', tempBuffer);
    Serial.flush();
    sprintf_P(tempBuffer, (PGM_P)F("IEC pins: ATN:%u CLK:%u DATA:%u RST:%u SRQIN:%u"), atnPin, clockPin, dataPin,
              resetPin, srqInPin);
    Log(Information, 'M', tempBuffer);
    sprintf_P(tempBuffer, (PGM_P)F("Arduino time set to: %04u-%02u-%02u.%02u:%02u:%02u"), year, month, day, hour, minute, second);
    Log(Information, 'M', tempBuffer);
} // waitForPeer
