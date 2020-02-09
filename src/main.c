#include <stdio.h>

#include "server/socketServer.h"
#include "utilTypes.h"


#define TEST_UDP
#ifdef TEST_UDP
#include "server/socketUDP.h"
#elif defined(TEST_TCP)
#include "server/socketTCP.h"
#endif


/**
1. UDP client-finding should be optional.
2. A way of letting the developer handle new connections would be nice.
**/


// Forward-declare our helper functions!
static void disconnectFunc(socketHandler *handler, const socketInfo *client);
static void bufferFunc(socketHandler *handler, const socketInfo *client, const char *buffer, const size_t bufferLength);


int main(int argc, char *argv[]){
	if(serverSetup()){
		socketServer server;
		socketServerConfig cfg;

		#ifdef TEST_UDP
		serverConfigInit(&cfg, SOCK_DGRAM, IPPROTO_UDP);

		if(serverInit(&server, cfg)){
			//

			serverCloseUDP(&server.connectionHandler);
		}
		#elif defined(TEST_TCP)
		serverConfigInit(&cfg, SOCK_STREAM, IPPROTO_TCP);

		if(serverInit(&server, cfg)){
			for(;;){
				int changedSockets = serverListenTCP(&server.connectionHandler);
				// An error has occurred.
				if(changedSockets < 0){
					break;
				}else{
					socketInfo *curClient = &server.connectionHandler.info[1];

					// Keep looping until we've found every client whose state has changed.
					while(changedSockets > 0){
						serverGetNextSocketTCP(curClient);
						char buffer[SERVER_MAX_BUFFER_SIZE];
						const int bufferLength = serverReceiveTCP(curClient, buffer);

						// The socket has disconnected.
						if(bufferLength == 0){
							disconnectFunc(&server.connectionHandler, curClient);
							serverDisconnectTCP(&server.connectionHandler, curClient);
							--changedSockets;

						// The socket has sent some data.
						}else if(bufferLength > 0){
							bufferFunc(&server.connectionHandler, curClient, buffer, bufferLength);
							--changedSockets;
						}
					}
				}
			}

			serverCloseTCP(&server.connectionHandler);
		}
		#endif
    }
    serverCleanup();


	puts("\n\nPress enter to exit.");
	getc(stdin);


    return(0);
}


static void disconnectFunc(socketHandler *handler, const socketInfo *client){
	printf("Client #%u has been disconnected.\n", client->id);
}

static void bufferFunc(socketHandler *handler, const socketInfo *client, const char *buffer, const size_t bufferLength){
	printf("Client #%u has sent: %s\n", client->id, buffer);
}