#ifndef socketTCP_h
#define socketTCP_h


#include <stddef.h>

#include "socketHandler.h"
#include "../utilTypes.h"


#define serverGetNextSocketTCP(info) for(; !socketInfoValid(info); ++info);


typedef struct socketInfo socketInfo;
typedef struct socketServer socketServer;


int serverListenTCP(socketHandler *handler);
int serverReceiveTCP(socketInfo *curInfo, char *buffer);
return_t serverSendTCP(const socketHandler *handler, const socketInfo *client, const char *buffer, const size_t bufferLength);
void serverDisconnectTCP(socketHandler *handler, socketInfo *client);
void serverCloseTCP(socketHandler *handler);


#endif