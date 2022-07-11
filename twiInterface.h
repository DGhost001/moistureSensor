#ifndef TWI_INTERFACE_H
#define TWI_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief twiSendChar will put the provided character into the sendbuffer
 * @return If character could be put into the send buffer
 */
bool twiSendChar(char);

/**
 * @brief twiCharAvailable retruns true if there is a character in the receive buffer
 * @return True - Character available
 */
bool twiCharAvailable( void );

/**
 * @brief twiReceiveChar Returns the received character, NULL if none was received
 * @return
 */
char twiReceiveChar( void );

/**
 * @brief twiInitialize Initializes the TWI Interface
 */
void twiInitialize(uint8_t);

/**
 * @brief twiSleep will enter IDLE, if USI allows it right now
 */
void twiSleep( void );

#endif
