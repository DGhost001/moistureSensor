SET(CMAKE_SYSTEM_NAME AVR)

#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)
# where is the target environment
SET(CMAKE_FIND_ROOT_PATH /usr/lib/gcc/avr )

SET(CMAKE_C_COMPILER avr-gcc)
SET(CMAKE_CXX_COMPILER avr-g++)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(CSTANDARD "-std=gnu99")
SET(CDEBUG "-gstabs")
SET(CWARN "-Wall -Wstrict-prototypes")
SET(CTUNING "-funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums")
SET(COPT "-O2")
SET(CMCU "-mmcu=attiny45")
SET(CDEFS "-DF_CPU=8000000 -D__AVR_ATtiny45__")
SET(CMCU2 "-mmcu=atmega16")
SET(CDEFS2 "-DF_CPU=3800000 -D__AVR_ATmega16__")

SET(CFLAGS "${CMCU} ${CDEBUG} ${CDEFS} ${CINCS} ${COPT} ${CWARN} ${CSTANDARD} ${CEXTRA}")
SET(CXXFLAGS "${CMCU} ${CDEFS} ${CINCS} ${COPT}")

SET(CFLAGS2 "${CMCU2} ${CDEBUG} ${CDEFS2} ${CINCS} ${COPT} ${CWARN} ${CSTANDARD} ${CEXTRA}")
SET(CXXFLAGS2 "${CMCU2} ${CDEFS2} ${CINCS} ${COPT}")

SET(CMAKE_C_FLAGS  ${CFLAGS})
SET(CMAKE_CXX_FLAGS ${CXXFLAGS}) 
