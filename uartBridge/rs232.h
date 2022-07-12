#ifndef RS232_H
#define RS232_H

#include <stdint.h>
#include <stdbool.h>

void rs232SendHexByte(const uint8_t byte);
void rs232SendString(const char *string);
bool rs232WriteByte(const uint8_t data);
uint8_t rs232ReadByte( void );
void rs232Init( void );

#endif
