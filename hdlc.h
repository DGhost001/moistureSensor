#ifndef HDLC_H
#define HDLC_H

#include <stdbool.h>
#include <stdlib.h>

bool hdlcSendBuffer(void const * const buffer, size_t const bufferSize);
bool hdlcReceiveBuffer(void *const buffer, size_t const bufferSize);

#endif
