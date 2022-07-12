
SET(CMAKE_C_FLAGS  ${CFLAGS2})
SET(CMAKE_CXX_FLAGS ${CXXFLAGS2}) 


SET(SOURCE
    main.c
    rs232.c
    rs232.h
    ../crc16.c
    ../hdlc.c
)

SET(HEADER
    )

SET(EXECUTABLE uartBridge)

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


add_custom_target(flash-uartBridge avr-objcopy -j .text -j .data -O ihex ${EXECUTABLE} ${EXECUTABLE}.ihex
                  COMMAND avrdude -c stk500 -P /dev/ttyUSB0 -p m16 -U flash:w:${EXECUTABLE}.ihex:i
                  DEPENDS ${EXECUTABLE}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  BYPRODUCTS ${EXECUTABLE}.ihex
                  )

add_custom_target(disasm-uartBridge avr-objdump -DS -z ${EXECUTABLE} 2>&1 > ${EXECUTABLE}.S
                  DEPENDS ${EXECUTABLE}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  BYPRODUCTS ${EXECUTABLE}.S
                  )    
    
    
 
