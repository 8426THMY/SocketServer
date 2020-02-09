#ifndef socketServer_h
#define socketServer_h


#include <stdint.h>

#include "socketHandler.h"
#include "../utilTypes.h"


typedef struct socketServer socketServer;
typedef struct socketServerConfig {
	int type;
	int protocol;
	char ip[46];
	uint16_t port;
} socketServerConfig;

typedef struct socketServer {
	socketHandler connectionHandler;

	int type;
	int protocol;
} socketServer;


#ifdef _WIN32
return_t serverSetup();
void serverCleanup();
#else
#define serverSetup() 1
#define serverCleanup() ;
#endif

void serverConfigInit(socketServerConfig *cfg, const int type, const int protocol);
return_t serverInit(socketServer *server, socketServerConfig cfg);


#endif