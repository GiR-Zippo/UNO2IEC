#ifndef GLOBAL_DEFINES_HPP
#define GLOBAL_DEFINES_HPP

typedef unsigned long ulong;

// This should be defined if the RESET line is soldered in the IEC DIN connector. When defined it will give the
// arduino to go into a reset state and wait for the CBM to become ready for communiction.
#define HAS_RESET_LINE

// Define this for speed increase when reading (filling serial buffer while transferring
// to CBM without interrupts off). It is experimental, stability needs to be checked
// further even though it seems to work just fine.
//#define EXPERIMENTAL_SPEED_FIX

// For serial communication.
#define DEFAULT_BAUD_RATE 115200
#define SERIAL_TIMEOUT_MSECS 1000
//#define DEBUG
//#define RESET_C64

#define ATN 30
#define CLK 31
#define DATA 32
#define SRQ 33
#define RES 34

#endif // GLOBAL_DEFINES_HPP
