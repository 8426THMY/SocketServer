#ifndef socketUDP_h
#define socketUDP_h


#include <stddef.h>

#include "socketHandler.h"
#include "../utilTypes.h"


typedef struct socketInfo socketInfo;
typedef struct socketServer socketServer;


return_t serverListenUDP(socketHandler *handler);
return_t serverSendUDP(const socketHandler *handler, const socketInfo *client, const char *buffer, const size_t bufferLength);
void serverDisconnectUDP(socketHandler *handler, socketInfo *client);
void serverCloseUDP(socketHandler *handler);


#endif