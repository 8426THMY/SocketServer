#include "socketUDP.h"


#include <stdlib.h>
#include <string.h>

#include "socketShared.h"


int serverReceiveUDP(socketServer *handler, socketInfo **client, char *buffer){
	socketInfo newInfo;
	const int bufferLength = (newInfo.addressSize = sizeof(struct sockaddr), recvfrom(
		socketHandlerMasterHandle(handler)->fd, buffer, SERVER_MAX_BUFFER_SIZE, 0,
		(struct sockaddr *)&newInfo.address, &newInfo.addressSize
	));

	// Check whether there were any errors that caused us to stop receiving.
	if(bufferLength < 0){
		const int errorID = serverGetLastError();
		if(errorID != EWOULDBLOCK && errorID != ECONNRESET){
			#ifdef SERVER_DEBUG
			serverPrintError("recvfrom()", errorID);
			#endif
		}
		*client = NULL;
	}else{
		socketInfo *curInfo = &handler->info[1];
		size_t remainingSockets = handler->nfds - 1;

		// Find the new socket if we've already received data from it.
		for(; remainingSockets > 0; --remainingSockets){
			serverGetNextSocket(curInfo);
			// If the socket already exists, exit the loop.
			if(
				newInfo.addressSize == curInfo->addressSize &&
				memcmp(&newInfo.address, &curInfo->address, newInfo.addressSize)
			){
				*client = curInfo;
				return(bufferLength);
			}
		}

		// If the socket is new, add it to the connection handler!
		{
			socketHandle newHandle;
			const return_t success = (
				newHandle.events  = POLLIN,
				newHandle.revents = 0,
				socketHandlerAdd(handler, &newHandle, &newInfo)
			);
			// The connection handler is full or could not be resized.
			if(success <= 0){
				#ifdef SERVER_DEBUG
				puts("Warning: Incoming data rejected - connection handler is full.\n");
				#endif

				return(-1);
			}

			// Keep a pointer to the new socket.
			*client = handler->lastInfo;
		}
	}

	return(bufferLength);
}

// Send data to a socket.
return_t serverSendUDP(const socketServer *handler, const socketInfo *client, const char *buffer, const size_t bufferLength){
	if(sendto(socketHandlerMasterHandle(handler)->fd, buffer, bufferLength, 0, (struct sockaddr *)&client->address, client->addressSize) < 0){
		#ifdef SOCKET_DEBUG
		serverPrintError("send()", serverGetLastError());
		#endif
		return(0);
	}

	return(1);
}

// Disconnect a socket.
void serverDisconnectUDP(socketServer *handler, socketInfo *client){
	socketHandlerRemove(handler, client);
}

// Shutdown the server.
void serverCloseUDP(socketServer *handler){
	socketInfo *curInfo = &handler->info[1];
	const socketInfo *lastInfo = &handler->info[handler->nfds];

	// Disconnect all of the clients.
	for(; curInfo < lastInfo; ++curInfo){
		serverDisconnectUDP(handler, curInfo);
	}

	serverClose(handler);
}

#if 0
return_t serverListenUDP(socketHandler *handler){
	// Keep receiving data while the buffer is not empty.
	for(;;){
		socketInfo newInfo;
		newInfo.addressSize = sizeof(struct sockaddr);

		newInfo.lastBufferLength = recvfrom(
			socketHandlerMasterHandle(handler)->fd, newInfo.lastBuffer, SERVER_MAX_BUFFER_SIZE, 0,
			(struct sockaddr *)&newInfo.address, &newInfo.addressSize
		);

		// Check whether there were any errors that caused us to stop receiving.
		if(newInfo.lastBufferLength < 0){
			const int errorID = serverGetLastError();
			if(errorID != EWOULDBLOCK && errorID != ECONNRESET){
				#ifdef SERVER_DEBUG
				serverPrintError("recvfrom()", errorID);
				#endif
				return(0);
			}

			break;

		// Otherwise, search the connection handler
		// for our socket and set its buffer.
		}else{
			socketInfo *curInfo = &handler->info[1];
			size_t remainingSockets = handler->nfds - 1;

			// Find the new socket if we've already received data from it.
			while(remainingSockets > 0){
				if(socketInfoValid(curInfo)){
					// If the socket already exists, exit the loop.
					if(
						newInfo.addressSize == curInfo->addressSize &&
						memcmp(&newInfo.address, &curInfo->address, newInfo.addressSize)
					){
						break;
					}
					--remainingSockets;
				}
				++curInfo;
			}

			// If the socket is new, add it to the connection handler!
			if(remainingSockets == 0){
				socketHandle newHandle;
				const return_t success = (
					newInfo.flags     = SERVER_SOCKET_INFO_CONNECTED,
					newHandle.events  = POLLIN,
					newHandle.revents = 0,
					socketHandlerAdd(handler, &newHandle, &newInfo)
				);
				// The connection handler is full or could not be resized.
				if(success <= 0){
					#ifdef SERVER_DEBUG
					puts("Warning: Incoming data rejected - connection handler is full.\n");
					#endif
				}

				// Keep a pointer to the new socket.
				curInfo = handler->lastInfo;
			}

			// Update the socket's state.
			curInfo->lastBufferLength = newInfo.lastBufferLength;
			memcpy(curInfo->lastBuffer, newInfo.lastBuffer, newInfo.lastBufferLength);
			flagsSet(curInfo->flags, SERVER_SOCKET_INFO_SENT_DATA);
		}
	}

	return(1);
}
#endif