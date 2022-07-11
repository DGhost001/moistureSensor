#include <avr/io.h>
#include <avr/interrupt.h>

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

static uint16_t readAnalogValue( void )
{
    uint16_t result = 0;

    ADCSRA |= (1<<ADSC); //Start conversion and clear Interrupt flag
    while((ADCSRA & (1<<ADSC)) != 0) {} //Wait for the conversion to finish

    result = (ADCL) | (ADCH << 8);   //Read the value

    return result;
}

static uint16_t getHumidityReading( void )
{
    uint16_t result = 0;

    ADMUX = 0x02; //Select PB4 (ADC2) and internal VCC Vref
    ADCSRA |= (1<<ADEN); //Enable the ADC

    for(unsigned i = 0; i<16; ++i) {
        result += readAnalogValue();
    }

    ADCSRA &= ~(1<<ADEN); //Disable the ADC again to save power

    return result;
}


static uint16_t getTemperatureReading( void )
{
    uint16_t result = 0;

    ADMUX = 0x0f | (1<<REFS1); //Select Temperature Sensor and internal 1.1V Vref
    ADCSRA |= (1<<ADEN); //Enable the ADC

    for(unsigned i = 0; i<16; ++i) {
        result += readAnalogValue();
    }

    ADCSRA &= ~(1<<ADEN); //Disable the ADC again to save power

    return result;
}


static void setupADC( void )
{
   ADCSRA = (1<<ADIF) | (1<<ADPS2); //Clear IF Flag, set Prescaler to 16, which should result in 125khz clock with 2Mhz CPU Clock
}

static void setupPowerSave( void )
{
   PRR = (1<<PRTIM1); //Disable Clocking of timer1 we don't need
}

static struct TelemetryCommand commandBuffer;

int main( void )
{
    setupClockPrescaler();
    setupPortBConfiguration();
    setupTimer0();
    twiInitialize(1);
    setupADC();
    setupPowerSave();
    sei();

    for(;;) {

        //Do we have successfully received an command?
        if(hdlcReceiveBuffer(&commandBuffer, sizeof(commandBuffer)))
        {
            switch (commandBuffer.cmdId) {
            case 0: //Ping ... we are alive ... so just loop back the data ...
            {
                hdlcSendBuffer(&commandBuffer, sizeof(commandBuffer));
                break;
            }

            case 1: //Request id
            {
                commandBuffer.parameter = (1<<8)| //FW Version 1
                                          (1<<1)| //Temperature Sensor
                                          (1<<0); //Humidity Sensor
                hdlcSendBuffer(&commandBuffer, sizeof(commandBuffer));
                break;
            }

            case 2: //Request a moisture measurement ...
            {
                commandBuffer.parameter = getHumidityReading();
                hdlcSendBuffer(&commandBuffer, sizeof(commandBuffer));
                break;
            }

            case 3: //Request a temperature measurement ...
            {
                commandBuffer.parameter = getTemperatureReading();
                hdlcSendBuffer(&commandBuffer, sizeof(commandBuffer));
                break;
            }

            default:
                break;
            }
        } else {
            if(!twiCharAvailable()) {
                twiSleep(); //Nothing to do ... just sleep a bit
            }
        }

    }
}
