#ifndef socketServer_h
#define socketServer_h


#include <stdio.h>

#include "socketHandler.h"


typedef struct socketServer socketServer;
typedef struct socketServer {
	socketHandler connectionHandler;

	int type;
	int protocol;

	char *lastBuffer;
	int lastBufferLength;
    size_t maxBufferSize;

	void (*discFunc)(socketServer *server, const socketInfo *client);
} socketServer;


unsigned char serverSetup();

unsigned char serverInit(socketServer *server, const int type, const int protocol, char *ip, size_t ipLength, unsigned short port, size_t bufferSize,
                         void (*discFunc)(socketServer *server, const socketInfo *client));

void serverCleanup();


#endif