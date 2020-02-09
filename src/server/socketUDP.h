#ifndef socketUDP_h
#define socketUDP_h


#include <stddef.h>

#include "socketServer.h"
#include "../utilTypes.h"


int serverReceiveUDP(socketServer *handler, socketInfo **client, char *buffer);
return_t serverSendUDP(const socketServer *handler, const socketInfo *client, const char *buffer, const size_t bufferLength);
void serverDisconnectUDP(socketServer *handler, socketInfo *client);
void serverCloseUDP(socketServer *handler);


#endif