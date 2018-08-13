#ifndef socketUDP_h
#define socketUDP_h


#include <stdio.h>

#include "socketServer.h"


unsigned char serverListenUDP(socketServer *server);
unsigned char serverSendUDP(const socketServer *server, const socketInfo *client, const char *buffer, const size_t bufferLength);
void serverDisconnectUDP(socketServer *server, const socketInfo *client);
void serverCloseUDP(socketServer *server);


#endif