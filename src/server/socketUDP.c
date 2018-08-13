#include "socketUDP.h"


#include <stdlib.h>
#include <string.h>

#include "socketShared.h"


unsigned char serverListenUDP(socketServer *server){
	//Store information pertaining to whoever sent the data!
	socketInfo tempInfo;
	tempInfo.id = 0;
	memset(&tempInfo.addr, 0, sizeof(tempInfo.addr));
	tempInfo.addrSize = sizeof(tempInfo.addr);

	//Fill the buffer with incoming data!
	server->lastBufferLength = recvfrom(server->connectionHandler.fds[0].fd, server->lastBuffer, server->maxBufferSize, 0, (struct sockaddr *)&tempInfo.addr, &tempInfo.addrSize);

	if(server->lastBufferLength >= 0){
		size_t i;
		//Loop through all the connected clients! We continue looping in case their are users to disconnect.
		for(i = 1; i < server->connectionHandler.size; ++i){
			//If this client has sent something, keep their I.D.!
			if(memcmp(&tempInfo.addr, &(server->connectionHandler.info[i].addr), tempInfo.addrSize) == 0){
				tempInfo.id = server->connectionHandler.info[i].id;

			//If this client has timed out, disconnect them!
			#warning "UDP timeout isn't implemented yet!"
			}else if(0){
				serverDisconnectUDP(server, &server->connectionHandler.info[i]);
				--i;
			}
		}

		//If this client isn't connected, add them to the connectionHandler!
		if(tempInfo.id == 0){
			handlerAdd(&server->connectionHandler, NULL, &tempInfo);
		}


		//The client has sent something, so add it to their buffer array!
		if(server->lastBufferLength > 0){
			server->lastBuffer[server->lastBufferLength] = '\0';
			//Add one to the length because we need to get the null-terminator.
			socketInfoBufferAdd(&server->connectionHandler.info[server->connectionHandler.idLinks[tempInfo.id]], server->lastBuffer, server->lastBufferLength + 1);

		//The client has disconnected, so disconnect them from our side!
		}else{
			serverDisconnectUDP(server, &server->connectionHandler.info[server->connectionHandler.idLinks[tempInfo.id]]);
		}

	//There was an error, so disconnect the client!
	}else{
		const int tempErrorID = serverGetLastError();
		if(tempErrorID != EWOULDBLOCK && tempErrorID != ECONNRESET){
			serverPrintError("recvfrom()", tempErrorID);
			serverDisconnectUDP(server, &tempInfo);

			return(0);
		}
	}


	return(1);
}

//Send a user a message!
unsigned char serverSendUDP(const socketServer *server, const socketInfo *client, const char *buffer, const size_t bufferLength){
	if(client->id < server->connectionHandler.capacity && server->connectionHandler.idLinks[client->id] != 0){
		//If the client exists, send the buffer to them!
		if(sendto(server->connectionHandler.fds[0].fd, buffer, bufferLength, 0, (struct sockaddr *)&client->addr, client->addrSize) >= 0){
			return(1);
		}else{
			serverPrintError("sendto()", serverGetLastError());
		}
	}else{
		printf("Error: Tried to send data to or from an invalid address.\n");
	}

	return(0);
}

//Disconnect a user!
void serverDisconnectUDP(socketServer *server, const socketInfo *client){
	if(client->id < server->connectionHandler.capacity && server->connectionHandler.idLinks[client->id] != 0){
		if(server->discFunc != NULL){
			(*server->discFunc)(server, client);
		}

		handlerRemove(&server->connectionHandler, client->id);
	}
}

//Shutdown the server!
void serverCloseUDP(socketServer *server){
	size_t i = server->connectionHandler.size;
	while(i > 1){
		--i;
		serverDisconnectUDP(server, &server->connectionHandler.info[i]);
	}
	handlerClear(&server->connectionHandler);

	free(server->lastBuffer);
}