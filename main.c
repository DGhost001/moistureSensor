#include <avr/io.h>
#include "twiInterface.h"
#include "hdlc.h"

struct TelemetryCommand {
    uint8_t cmdId;
    uint8_t cmdTag;
    uint16_t parameter;
};

static void setupClockPrescaler( void )
{
    CLKPR  = (1<<CLKPCE);
    CLKPR  = (1<<CLKPS1); //Setup a Clockspeed of 2Mhz so we get 1Mhz Timeroutput ...
}

static void setupTimer0( void )
{
    TCCR0A = (1<<COM0B0) | //Toggle OC0B on compare
             (1<<WGM01);   //Enable CTC mode

    OCR0A  = 0;            //Should this provide a div by 2?
    OCR0B  = 0;

    TCCR0B = (1<<CS00);     //Maximum IO Clock speed, no prescaler
}

static void setupPortBConfiguration( void )
{
    DDRB   = (1<<DDB1);    //Put Port B Pin1 into output mode
}

static uint16_t getHumidityReading( void )
{
    return 0;
}


static struct TelemetryCommand commandBuffer;

int main( void )
{
    setupClockPrescaler();
    setupPortBConfiguration();
    setupTimer0();
    twiInitialize(1);

    for(;;) {

        //Do we have successfully received an command?
        if(hdlcReceiveBuffer(&commandBuffer, sizeof(commandBuffer)))
        {
            switch (commandBuffer.cmdId) {
            case 1:
            {
                commandBuffer.parameter = getHumidityReading();
                hdlcSendBuffer(&commandBuffer, sizeof(commandBuffer));
                break;
            }
            default:
                break;
            }
        }

    }
}
