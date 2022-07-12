#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

#include "rs232.h"
#include "../crc16.h"
#include "../hdlc.h"

#ifndef __AVR_ATmega16__
    #error WRONG Microcontroller
#endif

struct TelemetryCommand {
    uint8_t cmdId;
    uint8_t cmdTag;
    uint16_t parameter;
};


static char buffer[32];
static int tindex;
static int rindex;
static struct TelemetryCommand cmd;

bool twiSendChar(uint8_t c)
{
    if(tindex < sizeof(buffer)) {
        buffer[tindex] = c;
        ++tindex;
    }
    return true;
}

bool twiCharAvailable( void )
{
    return rindex > 0;
}

char twiReceiveChar( void )
{
    char result = 0;
    if(rindex > 0) {
        rindex --;
        result = buffer[rindex];

    }

    return result;
}


void twiSleep( void )
{
}


static bool readCommand( void )
{
    memset(buffer,0,sizeof(buffer));
    uint8_t byte = rs232ReadByte();
    uint8_t index = 0;

    rs232SendString("\n>> ");

    while(byte != '\r')
    {
        if(byte != 0 && index < sizeof(buffer)) {
            rs232WriteByte(byte);
            buffer[index] = byte;
            ++index;
        }

        byte = rs232ReadByte();
    }

    rs232SendString("\n");
    return index < sizeof(buffer);
}

static char* nextParameter(char* buff)
{
    while((*buff)!=0 && isspace(*buff)) ++buff;

    return buff;
}


#define TWI_CLEAR_STATUS 3

static void twiInit( void )
{
    PORTC = 0xff;
    TWBR = 255;
    TWSR = TWI_CLEAR_STATUS;
    TWCR = (1<<TWEN);
}

#define TWI_START (1<<TWEN) | (1<<TWSTA) | (1<<TWINT)
#define TWI_ADDR  (1<<TWEN) | (1<<TWINT)
#define TWI_DATA  (1<<TWEN) | (1<<TWINT)
#define TWI_STOP  (1<<TWEN) | (1<<TWSTO) | (1<<TWINT)
#define TWI_DATA_ACK  (1<<TWEN) | (1<<TWINT) | (1<<TWEA)

static int twiSendBuffer(unsigned address, uint8_t * buffer, size_t bufflen)
{
    TWSR = TWI_CLEAR_STATUS;
    TWCR  = TWI_START;

    rs232SendString("Sending Start..\n");

    while(!(TWCR & (1<<TWINT))) {} //Wait for the hardware to complete
    if(TW_STATUS != TW_START) { return 1; } //No Start Send ...

    TWSR = TWI_CLEAR_STATUS;
    TWDR  = (address << 1) & 0xfe;
    TWCR  = TWI_ADDR;

    rs232SendString("Sending Address..\n");
    while(!(TWCR & (1<<TWINT))) {} //Wait for the hardware to complete
    if(TW_STATUS != TW_MT_SLA_ACK) { return 2; } //Got NACK ...

    rs232SendString("Sending Data..\n");
    for(size_t i=0; i<bufflen; ++i) {
        TWSR = TWI_CLEAR_STATUS;
        TWDR  = buffer[i];
        TWCR  = TWI_DATA;
        while(!(TWCR & (1<<TWINT))) {} //Wait for the hardware to complete
        if(TW_STATUS != TW_MT_DATA_ACK) { return 3; } //Got NACK ...
    }

    rs232SendString("Sending Stop..\n");
    TWSR = TWI_CLEAR_STATUS;
    TWDR  = 0;
    TWCR  = TWI_STOP;

    return 0;
}

static int twiReceiveBuffer(unsigned address, uint8_t * buffer, size_t bufflen)
{
    TWSR = TWI_CLEAR_STATUS;
    TWCR  = TWI_START;

    rs232SendString("Sending Start..\n");
    while(!(TWCR & (1<<TWINT))) {} //Wait for the hardware to complete
    if(TW_STATUS != TW_START) { TWCR = TWI_STOP; return 1; } //No Start Send ...

    TWSR = TWI_CLEAR_STATUS;
    TWDR  = (address << 1) & 0xfe | 1;
    TWCR  = TWI_ADDR;

    rs232SendString("Sending Address..\n");
    while(!(TWCR & (1<<TWINT))) {} //Wait for the hardware to complete
    if(TW_STATUS != TW_MR_SLA_ACK) { TWCR = TWI_STOP; return 2; } //Got NACK ...

    rs232SendString("Getting Data..\n");
    for(size_t i=0; i<bufflen; ++i) {
        TWSR = TWI_CLEAR_STATUS;
        TWCR  = ((i + 1) >= bufflen) ?  TWI_DATA : TWI_DATA_ACK;
        while(!(TWCR & (1<<TWINT))) {} //Wait for the hardware to complete
        if((((i + 1) < bufflen)) && (TW_STATUS != TW_MR_DATA_ACK)) { TWCR = TWI_STOP; return 3; } //Got NACK ...
        buffer[i] = TWDR;
    }

    rs232SendString("Sending Stop..\n");
    TWSR = TWI_CLEAR_STATUS;
    TWDR  = 0;
    TWCR  = TWI_STOP;

    return 0;
}

static void parseCommand( void )
{
    tindex = 0;
    char *buff = buffer;
    char *next;

    buff = nextParameter(buff);
    next = nextParameter(buff+1);

    switch (*buff) {
        case 'r':
    {
        unsigned address;
        unsigned length;
        if(sscanf(next,"%x %d", &address, &length) == 2)
        {
            if(length > sizeof(buffer)) {
                rs232SendString("Length too long.\n");
            } else {
                unsigned result = twiReceiveBuffer(address, buffer, length);

                for(size_t i = 0; i<length && result == 0; ++i)
                {
                    rs232SendHexByte(buffer[i]);
                    rs232WriteByte(' ');
                }

                sprintf(buffer,"\nResult: %d\n", result);
                rs232SendString(buffer);
            }
        }else {
            rs232SendString("Not enougth parameter for command.\n");
        }
        break;
    }
    case 'c':
    {
        unsigned address;
        unsigned id;
        unsigned tag;

        if(sscanf(next,"%x %d %d %d", &address, &id, &tag, &cmd.parameter) == 4)
        {
            cmd.cmdId = id;
            cmd.cmdTag = tag;
            hdlcSendBuffer(&cmd, sizeof(cmd));
            unsigned result = twiSendBuffer(address, buffer, tindex);

            for(size_t i = 0; i<tindex; ++i)
            {
                rs232SendHexByte(buffer[i]);
                rs232WriteByte(' ');
            }

            sprintf(buffer,"\nResult: %d\n", result);
            rs232SendString(buffer);

        }else {
            rs232SendString("Not enougth parameter for command.\n");
        }

        break;
    }

        case 'w':
    {

        break;
    }
        case 's':
    {

        break;
    }
    }




}




int main( void )
{
    cli();
    rs232Init();
    twiInit();
    sei();
    rs232SendString("RS232 - TWI Bridge\n");
#if 0
    cmd.cmdId = 1;
    cmd.cmdTag = 1;
    cmd.parameter = 1234;
    hdlcSendBuffer(&cmd, sizeof(cmd));
    twiSendBuffer(1, buffer, tindex);
#endif

    uint8_t i=0;
    for(;;) {

        if(readCommand())
        {
            parseCommand();
        } else {
            rs232SendString("Command too long!\n");
        }


    }
}
