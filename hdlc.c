#include "hdlc.h"
#include "twiInterface.h"
#include "crc16.h"


static bool sendChar(uint8_t const c)
{
    bool result;
    if(c == 0x7e || c == 0x7f)
    {
        result = twiSendChar(0x7f);
        result = result && twiSendChar(c ^0x20);
    } else
    {
        result = twiSendChar(c);
    }

    return result;
}

static uint8_t receiveChar( uint8_t c )
{
    uint8_t result = c;

    if(result == 0x7f) {
        while(!twiCharAvailable()) { twiSleep(); }
        result = twiReceiveChar() ^ 0x20;
    }

    return result;
}

bool hdlcSendBuffer(void const * const buffer, size_t const bufferSize)
{
    uint16_t crc = computeCrc(buffer, bufferSize);

    bool result = twiSendChar(0x7e);
    for(unsigned i=0; i<bufferSize && result; ++i)
    {
        result = sendChar(((uint8_t*)buffer)[i]);
    }

    sendChar(crc >> 8);
    sendChar(crc & 0xff);

    result = result && twiSendChar(0x7e);

    return result;
}

bool hdlcReceiveBuffer(void *const buffer, size_t const bufferSize)
{
    bool inFrame = false;
    uint16_t crc = 0;
    size_t dataReceived = 0;

    while(1)
    {
        while(!twiCharAvailable()) { twiSleep(); }

        uint8_t data = twiReceiveChar();

        if(0x7e == data)
        {
            if(!inFrame) {
                inFrame = true;
            } else {
                if(dataReceived > bufferSize) {
                    break; //Frame received
                } //else frame was to short or transmission corrupted, so we wait for more data...
            }
        } else {
            data = receiveChar(data);

            if(dataReceived < bufferSize) {
                ((uint8_t*)buffer)[dataReceived] = data;
            } else if(dataReceived - bufferSize < 2) {
                crc = (crc << 8) | data;
            }
            ++dataReceived;
        }
    }

   uint16_t crcBuffer = computeCrc(buffer, bufferSize);

   return crcBuffer == crc;
}
