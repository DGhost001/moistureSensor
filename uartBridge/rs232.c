#include "rs232.h"

#include <avr/io.h>
#include <avr/iom16.h>
#include <avr/interrupt.h>

#include <stdint.h>
#include <stdio.h>

enum{
  maxBufferSize = 64
};

struct Buffer{
    uint8_t buffer[maxBufferSize];
    unsigned read;
    unsigned write;
};

static volatile struct Buffer rxBuffer_;

static unsigned next(unsigned const i)
{
    return (i + 1) % maxBufferSize;
}

void rs232Init( void )
{
    rxBuffer_.read = 0;
    rxBuffer_.write = 0;

    DDRB  = 0xff;
    UCSRA = 0x0; //Disable Multiprocessor and 2x Speed
    //UCSRA = (1<<U2X); //Enable double clock
    UBRRH = 0x0;
    //UBRRL = 25;  // 25 = 19.2 kbps
    UBRRL = 12;    //38.400 @ 8Mhz
    //UBRRL = 3;   // 155.2 kbps (+8% Fehler)
    UCSRC = (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0); // no parity, 1 stop, 8 data
    UCSRB = (1<<RXEN) | (1<<TXEN) | (1 << RXCIE); //Enable RX, TX, No Interrups as we poll it in this config
}

void debugNumber(unsigned const number, unsigned const number2)
{
    cli();
    char buffer[20];
    memset(buffer,0,sizeof(buffer));
    sprintf(buffer, "DBG: %d -> %d\r\n",number, number2);
    rs232SendString(buffer);
    sei();
}



uint8_t rs232ReadByte( void )
{
    uint8_t result = 0;

    //debugNumber(1,rxBuffer_.read);
    //debugNumber(2,rxBuffer_.write);

    if(rxBuffer_.read != rxBuffer_.write)
    {
        result = rxBuffer_.buffer[rxBuffer_.read];
        rxBuffer_.read=next(rxBuffer_.read);
    }

    return result;
}

static void pollByte(uint8_t const byte)
{
    while((UCSRA & (1<<UDRE)) == 0) {}
    UDR = byte;
}

bool rs232WriteByte(const uint8_t data)
{
    pollByte(data);
    return true;
}

void rs232SendString(const char *string)
{
    const char *tmp = string;

    while((*tmp) != 0)
    {
        if(*tmp == '\n' ) {
            rs232WriteByte('\r');
        }

        rs232WriteByte((*tmp));
        tmp++;
    }
}

void rs232SendHexByte(const uint8_t byte)
{
    const char hex[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    while(!rs232WriteByte(hex[(byte>>4) & 0xF])) {}
    while(!rs232WriteByte(hex[(byte & 0xF)])) {}
}

ISR(USART_RXC_vect)
{
    PORTB = 0xff;
    unsigned nwrite = next(rxBuffer_.write);
    if(nwrite != rxBuffer_.read) {
        rxBuffer_.buffer[rxBuffer_.write] = UDR;
        rxBuffer_.write = nwrite;
    }
    PORTB = 0x00;
}
