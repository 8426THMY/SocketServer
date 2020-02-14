#ifndef socketTCP_h
#define socketTCP_h


#include <stddef.h>

#include "socketServer.h"
#include "../utilTypes.h"


int serverListenTCP(socketServer *const restrict handler);
int serverReceiveTCP(socketInfo *const restrict client, char *const restrict buffer);
return_t serverSendTCP(const socketServer *const restrict handler, const socketInfo *const restrict client, const char *const restrict buffer, const size_t bufferLength);
void serverDisconnectTCP(socketServer *const restrict handler, socketInfo *const restrict client);
void serverCloseTCP(socketServer *const restrict handler);


#endif