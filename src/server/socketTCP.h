#ifndef socketTCP_h
#define socketTCP_h


#include <stddef.h>

#include "socketServer.h"
#include "../utilTypes.h"


int serverListenTCP(socketServer *handler);
int serverReceiveTCP(socketInfo *client, char *buffer);
return_t serverSendTCP(const socketServer *handler, const socketInfo *client, const char *buffer, const size_t bufferLength);
void serverDisconnectTCP(socketServer *handler, socketInfo *client);
void serverCloseTCP(socketServer *handler);


#endif