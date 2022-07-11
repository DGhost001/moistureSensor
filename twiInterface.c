#include "twiInterface.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <stdint.h>

#define SDA DDB0
#define SCL DDB2

enum{
  maxBufferSize = 8
};

enum TwiStatus {
    twiWaitForStart,
    twiWaitForAddress,
    twiWaitForData,
    twiSendData,
    twiSendAck,
    twiWaitAck,
};

struct Buffer{
    uint8_t buffer[maxBufferSize];
    unsigned read;
    unsigned write;
};

static struct Buffer rxBuffer_;
static struct Buffer txBuffer_;

static enum TwiStatus internalState_;
static uint8_t ownAddress_;

static void usiSetUsIsr(unsigned const bits)
{
    USISR = (0 << USISIF) | (1 << USIOIF) | (1 << USIPF) | (1 << USIDC) | /* Clear all flags, except Start Cond  */
            ((0xf - (bits&0xf)) << USICNT0); /* set USI counter to shift n bit. */
}

static void usiOutput(bool const out)
{
    if(out) {
        DDRB |= (1 << SDA); /* Set SDA as output  */
    } else {
        DDRB &= ~(1 << SDA); /* Set SDA as intput */
    }
}


static void usiPrepareAck( void )
{
    USIDR = 0;          /* Prepare ACK                         */
    usiOutput(true);
    usiSetUsIsr(1);
}

static void usiPrepareNack( void )
{
    usiOutput(false);
    usiSetUsIsr(1);
}

static void usiPrepareReadAck( void )
{
    USIDR = 0;                       /* Prepare ACK        */
    usiOutput(false);
    usiSetUsIsr(1);
}

static void usiSetToStartCondition( void )
{
    usiOutput(false);
    USICR = (1 << USISIE) | (0 << USIOIE) | /* Enable Start Condition Interrupt. Disable Overflow Interrupt.*/
            (1 << USIWM1) | (0 << USIWM0) | /* Set USI in Two-wire mode. No USI Counter overflow hold.      */
            (1 << USICS1) | (0 << USICS0) | (0 << USICLK) | /* Shift Register Clock Source = External, positive edge */
            (0 << USITC);
    usiSetUsIsr(8);
}

static void usiSetToSendData( void )
{
    usiOutput(true);
    usiSetUsIsr(8);
}

static void usiSetToReadData( void )
{
    usiOutput(false);
    usiSetUsIsr(8);
}

static unsigned next(unsigned i)
{
    return (i + 1) % maxBufferSize;
}

bool twiSendChar(char const c)
{
    bool result = false;

    unsigned const nextWrite = next(txBuffer_.write);

    if( nextWrite != txBuffer_.read) {
        txBuffer_.buffer[txBuffer_.write] = c;
        txBuffer_.write = nextWrite;
        result = true;
    }

    return result;
}

bool twiCharAvailable( void )
{
    return rxBuffer_.write != rxBuffer_.read;
}

char twiReceiveChar( void )
{
    char result = 0;
    if(twiCharAvailable()) {
        result = rxBuffer_.buffer[rxBuffer_.read];
        rxBuffer_.read = next(rxBuffer_.read);
    }

    return result;
}

void twiInitialize(uint8_t const address)
{
    internalState_ = twiWaitForStart;
    ownAddress_ = address;
}

ISR(USI_START_vect)
{
    USISR = 0; //Reset the USI Status, with the detection of the Start Condition
    USICR = USICR | (1<<USIOIE); //Enable the overflow condition
    internalState_ = twiWaitForAddress; //We received the START, now wait for the address
}

ISR(USI_OVF_vect)
{

}
