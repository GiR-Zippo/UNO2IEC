# This configuration file outlines some of the ways Arduino CMake
# can be used. For a detailed explenation of all the options please
# reade README.rst.

file(GLOB_RECURSE sources_Drivers Drivers/*.cpp Drivers/*.hpp)
file(GLOB_RECURSE sources_IEC IECDriver/*.cpp IECDriver/*.h)
file(GLOB_RECURSE sources_Interface Interface/*.cpp Interface/*.h Interface/*.hpp)
file(GLOB_RECURSE sources_Log Log/*.cpp Log/*.h)
file(GLOB_RECURSE sources_Misc misc/*.cpp misc/*.h)

# Some installations have renamed the example directories
set(BLINK_SKETCH
${BLINK_SKETCH}
${sources_Drivers}
${sources_IEC}
${sources_Interface}
${sources_Log}
${sources_Misc}
)

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Drivers
  ${CMAKE_CURRENT_SOURCE_DIR}/IECDriver
  ${CMAKE_CURRENT_SOURCE_DIR}/Interface
  ${CMAKE_CURRENT_SOURCE_DIR}/Log
  ${CMAKE_CURRENT_SOURCE_DIR}/misc
)

generate_arduino_library(uno2iec_lib
    SRCS ${BLINK_SKETCH}
    HDRS ${BLINK_SKETCH}
    BOARD mega2560)

generate_arduino_firmware(uno2iec
    SRCS main.cpp
    LIBS uno2iec_lib
    PORT  /dev/ttyACM0
    SERIAL avrispmkii @SERIAL_PORT@
    BOARD mega2560)
    

