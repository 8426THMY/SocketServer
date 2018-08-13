#include "socketTCP.h"


#include <stdlib.h>
#include <string.h>

#include "socketShared.h"


unsigned char serverListenTCP(socketServer *server){
	//Check if the any of the sockets have changed state.
	int changedSockets = pollFunc(server->connectionHandler.fds, server->connectionHandler.size, SERVER_POLL_TIMEOUT);
	//If the result was SOCKET_ERROR, print the error code!
	if(changedSockets == SOCKET_ERROR){
		serverPrintError(SERVER_POLL_FUNC, serverGetLastError());

		return(0);

	//Otherwise, if some sockets changed, handle them!
	}else if(changedSockets > 0){
		//If the master socket has returned with POLLIN, we're ready to accept a new connection!
		if(server->connectionHandler.fds[0].revents & POLLIN){
			//Store information pertaining to whoever sent the data!
			struct pollfd tempFD;
			socketInfo tempInfo;
			memset(&tempInfo.addr, 0, sizeof(tempInfo.addr));
			tempInfo.addrSize = sizeof(tempInfo.addr);

			tempFD.fd = accept(server->connectionHandler.fds[0].fd, (struct sockaddr *)&tempInfo.addr, &tempInfo.addrSize);

			//If the connection was accepted successfully, add it to the connectionHandler!
			if(tempFD.fd != INVALID_SOCKET){
				tempFD.events = POLLIN;
				tempFD.revents = 0;

				handlerAdd(&server->connectionHandler, &tempFD, &tempInfo);
			}else{
				serverPrintError("accept()", serverGetLastError());
			}

			//Reset the master socket's return events!
			server->connectionHandler.fds[0].revents = 0;

			--changedSockets;
		}

		size_t i;
		//Now check if the other sockets have changed!
		for(i = 1; changedSockets > 0 && i < server->connectionHandler.size; ++i){
			//Handle the client's events!
			if(server->connectionHandler.fds[i].revents != 0){
				//Store this stuff in case some clients disconnect!
				const size_t oldID = server->connectionHandler.info[i].id;
				const size_t oldSize = server->connectionHandler.size;

				//Check if the client has sent something!
				if(server->connectionHandler.fds[i].revents & POLLIN){
					//Check what they've sent!
					server->lastBufferLength = recv(server->connectionHandler.fds[i].fd, server->lastBuffer, server->maxBufferSize, 0);

					//The client has sent something, so add it to their buffer array!
					if(server->lastBufferLength > 0){
						server->lastBuffer[server->lastBufferLength] = '\0';
						//Add one to the length because we need to get the null-terminator.
						socketInfoBufferAdd(&server->connectionHandler.info[i], server->lastBuffer, server->lastBufferLength + 1);
					}else{
						//The client has disconnected, so disconnect them from our side!
						if(server->lastBufferLength == 0){
							serverDisconnectTCP(server, &server->connectionHandler.info[i]);

						//There was an error, so disconnect the client!
						}else{
							serverPrintError("recv()", serverGetLastError());
							serverDisconnectTCP(server, &server->connectionHandler.info[i]);
						}
					}
				}

				//Check if we haven't disconnected the client!
				if(server->connectionHandler.idLinks[oldID] != 0){
					//If they've hung up, disconnect them from our end!
					if(server->connectionHandler.fds[i].revents & POLLHUP){
						serverDisconnectTCP(server, &server->connectionHandler.info[i]);

					//Otherwise, clear their return events!
					}else{
						server->connectionHandler.fds[i].revents = 0;
					}
				}
				//Move the iterator back if we have to!
				if(oldSize > server->connectionHandler.size){
					//If the current client didn't disconnect, just find their new index!
					if(server->connectionHandler.idLinks[oldID] != 0){
						i = server->connectionHandler.idLinks[oldID];

					//Otherwise, skip back using the difference in size. It's a bit inaccurate, but that's fine.
					}else{
						i += server->connectionHandler.size;
						if(i > oldSize){
							i -= oldSize;
						}else{
							i = 1;
						}
					}
				}

				--changedSockets;

			//If the client has timed out, disconnect them!
			#warning "TCP timeout isn't implemented yet!"
			}else if(0){
				serverDisconnectTCP(server, &server->connectionHandler.info[i]);
				--i;
			}
		}
	}


	return(1);
}

//Send a user a message!
unsigned char serverSendTCP(const socketServer *server, const socketInfo *client, const char *buffer, const size_t bufferLength){
	size_t clientPos = server->connectionHandler.idLinks[client->id];
	if(client->id < server->connectionHandler.capacity && clientPos != 0){
		//If the client exists, send the buffer to them!
		if(send(server->connectionHandler.fds[clientPos].fd, buffer, bufferLength, 0) >= 0){
			return(1);
		}else{
			serverPrintError("send()", serverGetLastError());
		}
	}else{
		printf("Error: Tried to send data to an invalid socket.\n");
	}

	return(0);
}

//Disconnect a user!
void serverDisconnectTCP(socketServer *server, const socketInfo *client){
	size_t clientPos = server->connectionHandler.idLinks[client->id];
	if(client->id < server->connectionHandler.capacity && clientPos != 0){
		if(server->discFunc != NULL){
			(*server->discFunc)(server, client);
		}

		socketclose(server->connectionHandler.fds[clientPos].fd);
		handlerRemove(&server->connectionHandler, client->id);
	}
}

//Shutdown the server!
void serverCloseTCP(socketServer *server){
	size_t i = server->connectionHandler.size;
	while(i > 1){
		--i;
		serverDisconnectTCP(server, &server->connectionHandler.info[i]);
	}
	socketclose(server->connectionHandler.fds[0].fd);
	handlerClear(&server->connectionHandler);

	free(server->lastBuffer);
}