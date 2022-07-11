#include "twiInterface.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <stdint.h>

#define SDA DDB0
#define SCL DDB2

#ifndef ISR
    #define ISR(X) void X(void)
#endif

enum{
  maxBufferSize = 16
};

enum TwiStatus {
    twiWaitForStart,
    twiWaitForAddress,
    twiWaitForData,
    twiSendData,
    twiSendAck,
    twiWaitAck,
    twiRequestAck,
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

static void usiWaitForStart( void )
{
    USICR = (1 << USISIE) | (0 << USIOIE) | /* Enable Start Condition Interrupt. Disable Overflow Interrupt.*/
            (1 << USIWM1) | (0 << USIWM0) | /* Set USI in Two-wire mode. No USI Counter overflow hold.      */
            (1 << USICS1) | (0 << USICS0) | (0 << USICLK) | /* Shift Register Clock Source = External, positive edge */
            (0 << USITC);
}

static void usiWaitForOverflow( void )
{
    USICR = (1 << USISIE) | (1 << USIOIE) | /* Enable Start Condition Interrupt. Disable Overflow Interrupt.*/
            (1 << USIWM1) | (1 << USIWM0) | /* Set USI in Two-wire mode. No USI Counter overflow hold.      */
            (1 << USICS1) | (0 << USICS0) | (0 << USICLK) | /* Shift Register Clock Source = External, positive edge */
            (0 << USITC);
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
    usiWaitForStart();
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

    PORTB |= (1<<SCL) | (1<<SDA); //Set SCL and SDA to high
    DDRB  |= (1<<SCL);            //Set SCL to Output

    /* Reset all Interrupts */
    USISR = 0xF0;

    usiSetToStartCondition();

}

ISR(USI_START_vect)
{
    internalState_ = twiWaitForAddress; //We received the START, now wait for the address

    unsigned sclPin;

    /* Wait here for SCL to also go low, or SDA to go high again */
    for(sclPin = PINB;
        (sclPin & (1<<SCL | 1 << SDA)) == (1<<SCL);
        sclPin = PINB
        ) {}

    if(sclPin & (1<<SCL)) {
        usiWaitForStart();
    } else {
        usiWaitForOverflow();
    }

    /* Reset all Interrupts  and flags*/
    USISR = 0xF0;
}

ISR(USI_OVF_vect)
{
    uint8_t const dataByte = USIDR;

    switch(internalState_)
    {
    case twiWaitForAddress:
    {
        //Check if we are addressed at all ...
        if((dataByte == 0) || ((dataByte >> 1) == ownAddress_))
        {
            //The address is our address ... do we have to send or receive?
            internalState_ = dataByte & 1 ? twiSendData : twiSendAck;
            usiPrepareAck(); //Acknowledge the reception of the Address
        } else {
            usiSetToStartCondition(); //We are not addressed ... so sleep again ...
            internalState_ = twiWaitForStart;
        }

        break;
    }

    case twiSendAck:
    {
        internalState_ = twiWaitForData;
        usiSetToReadData();
        break;
    }
    case twiWaitForData:
    {
        internalState_ = twiSendAck;
        unsigned const nextWrite = next(rxBuffer_.write);

        if(nextWrite != rxBuffer_.read)
        {
            rxBuffer_.buffer[rxBuffer_.write] = dataByte;
            rxBuffer_.write = nextWrite;
            usiPrepareAck();
        } else {
            usiPrepareNack();
        }

        break;
    }

    case twiWaitAck:
    {
        if(dataByte) //We received an NACK
        {
            usiSetToStartCondition(); //Go into start condition again ...
            break;
        }
    } //fall through
    case twiSendData:
    {
        //Do we have data ...
        if(txBuffer_.read != txBuffer_.write)
        {
            USIDR = txBuffer_.buffer[txBuffer_.read];
            txBuffer_.read = next(txBuffer_.read);
            internalState_ = twiRequestAck;
            usiSetToSendData();
        } else {
            internalState_ = twiWaitForStart;
            usiSetToStartCondition();
        }
        break;
    }

    case twiRequestAck:
    {
        internalState_ = twiWaitAck;
        usiPrepareReadAck();
        break;
    }

    default:
        internalState_ = twiWaitForStart;
        usiSetToStartCondition();
        break;
    }
}
