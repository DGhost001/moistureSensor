cmake_minimum_required(VERSION 2.8)

project(humiditySensor)
include_directories(${PROJECT_SOURCE_DIR})

SET(SOURCE
    main.c
)

SET(HEADER
    )

SET(EXECUTABLE humiditySensor)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${EXECUTABLE}
        ${SOURCE}
        ${HEADER}
)

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND avr-size --format=berkeley ${EXECUTABLE}
    COMMAND readelf -S ${EXECUTABLE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

add_custom_target(flash avr-objcopy -j .text -j .data -O ihex ${EXECUTABLE} ${EXECUTABLE}.ihex
                  COMMAND avrdude -c stk500 -P /dev/ttyUSB0 -p t45 -U flash:w:${EXECUTABLE}.ihex:i
                  DEPENDS ${EXECUTABLE}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  BYPRODUCTS ${EXECUTABLE}.ihex
                  )

add_custom_target(disasm avr-objdump -DS -z ${EXECUTABLE} 2>&1 > ${EXECUTABLE}.S
                  DEPENDS ${EXECUTABLE}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  BYPRODUCTS ${EXECUTABLE}.S
                  )