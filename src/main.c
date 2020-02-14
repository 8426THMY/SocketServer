#include <stdio.h>

#include "server/socketServer.h"
#include "utilTypes.h"


#define TEST_TCP
#ifdef TEST_UDP
#include "server/socketUDP.h"
#elif defined(TEST_TCP)
#include "server/socketTCP.h"
#endif


/**
UDP client-finding should be optional.
**/


// Forward-declare our helper functions!
static void connectFunc(const socketInfo *const restrict client);
#ifdef TEST_TCP
static void disconnectFunc(const socketInfo *const restrict client);
#endif
static void bufferFunc(const socketInfo *const restrict client, const char *const restrict buffer, const size_t bufferLength);


int main(int argc, char *argv[]){
	if(serverSetup()){
		socketServer server;
		socketServerConfig cfg;

		#ifdef TEST_UDP
		serverConfigInit(&cfg, SOCK_DGRAM, IPPROTO_UDP);

		if(serverInit(&server, cfg)){
			for(;;){
				socketInfo *curClient;
				const size_t nfdsOld = server.nfds;
				char buffer[SERVER_MAX_BUFFER_SIZE];
				const int bufferLength = serverReceiveUDP(&server, &curClient, buffer);

				// A new socket has just connected.
				if(server.nfds != nfdsOld){
					connectFunc(server.lastInfo);
				}
				// The socket has sent some data.
				if(bufferLength >= 0){
					bufferFunc(curClient, buffer, bufferLength);
				}
			}

			serverCloseUDP(&server);
		}
		#elif defined(TEST_TCP)
		serverConfigInit(&cfg, SOCK_STREAM, IPPROTO_TCP);

		if(serverInit(&server, cfg)){
			for(;;){
				const size_t nfdsOld = server.nfds;
				int changedSockets = serverListenTCP(&server);
				// An error has occurred.
				if(changedSockets < 0){
					break;
				}else{
					socketInfo *curClient = &server.info[1];

					// A new socket has just connected.
					if(server.nfds != nfdsOld){
						connectFunc(server.lastInfo);
					}

					// Keep looping until we've found every client whose state has changed.
					while(changedSockets > 0){
						serverGetNextSocket(curClient);
						char buffer[SERVER_MAX_BUFFER_SIZE];
						const int bufferLength = serverReceiveTCP(curClient, buffer);

						// The socket has disconnected.
						if(bufferLength == 0){
							disconnectFunc(curClient);
							serverDisconnectTCP(&server, curClient);
							--changedSockets;

						// The socket has sent some data.
						}else if(bufferLength > 0){
							bufferFunc(curClient, buffer, bufferLength);
							--changedSockets;
						}
					}
				}
			}

			serverCloseTCP(&server);
		}
		#endif
    }
    serverCleanup();


	puts("\n\nPress enter to exit.");
	getc(stdin);


    return(0);
}


static void connectFunc(const socketInfo *const restrict client){
	printf("Client #%u has connected.\n", client->id);
}

#ifdef TEST_TCP
static void disconnectFunc(const socketInfo *const restrict client){
	printf("Client #%u has disconnected.\n", client->id);
}
#endif

static void bufferFunc(const socketInfo *const restrict client, const char *const restrict buffer, const size_t bufferLength){
	printf("Client #%u has sent: %s\n", client->id, buffer);
}