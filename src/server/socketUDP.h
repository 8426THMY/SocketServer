#ifndef socketUDP_h
#define socketUDP_h


#include <stddef.h>

#include "socketServer.h"
#include "../utilTypes.h"


int serverReceiveUDP(socketServer *const restrict handler, socketInfo **const restrict client, char *const restrict buffer);
return_t serverSendUDP(const socketServer *const restrict handler, const socketInfo *const restrict client, const char *const restrict buffer, const size_t bufferLength);
void serverDisconnectUDP(socketServer *const restrict handler, socketInfo *const restrict client);
void serverCloseUDP(socketServer *const restrict handler);


#endif