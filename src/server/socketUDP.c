#include "socketUDP.h"


#include <stdlib.h>
#include <string.h>

#include "socketShared.h"


return_t serverListenUDP(socketHandler *handler){
	return(0);
}

// Send data to a socket.
return_t serverSendUDP(const socketHandler *handler, const socketInfo *client, const char *buffer, const size_t bufferLength){
	if(sendto(socketHandlerMasterHandle(handler)->fd, buffer, bufferLength, 0, (struct sockaddr *)&client->address, client->addressSize) < 0){
		#ifdef SOCKET_DEBUG
		serverPrintError("send()", serverGetLastError());
		#endif
		return(0);
	}

	return(1);
}

// Disconnect a socket.
void serverDisconnectUDP(socketHandler *handler, socketInfo *client){
	socketHandlerRemove(handler, client);
}

// Shutdown the server.
void serverCloseUDP(socketHandler *handler){
	socketInfo *curInfo = &handler->info[1];
	const socketInfo *lastInfo = &handler->info[handler->nfds];

	// Disconnect all of the clients.
	for(; curInfo < lastInfo; ++curInfo){
		serverDisconnectUDP(handler, curInfo);
	}

	socketHandlerDelete(handler);
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

		// If no data was received, we can exit the loop.
		if(newInfo.lastBufferLength <= 0){
			// Check whether there were any errors that caused us to stop receiving.
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
					newHandle.events  = POLLIN | POLLHUP,
					newHandle.revents = 0,
					socketHandlerAdd(handler, &newHandle, &newInfo)
				);
				#ifdef SERVER_SOCKET_HANDLER_REALLOCATE
				// Memory allocation failure.
				if(success < 0){
					return(-1);
				}
				#else
				// The connection handler is full.
				if(success == 0){
					#ifdef SERVER_DEBUG
					puts("Warning: Rejected data from new socket - connection handler is full.\n");
					#endif
					continue;
				}
				#endif

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