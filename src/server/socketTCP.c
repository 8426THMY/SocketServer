#include "socketTCP.h"


#include <stdlib.h>
#include <string.h>

#include "socketShared.h"


/*
** Poll the file descriptors and return the number of
** sockets that changed state or -1 if there was an error.
*/
int serverListenTCP(socketServer *const restrict handler){
	const int changedSockets = pollFunc(handler->handles, handler->nfds, SERVER_POLL_TIMEOUT);
	// Make sure there wasn't an error while polling.
	if(changedSockets == SOCKET_ERROR){
		#ifdef SERVER_DEBUG
		serverPrintError(SERVER_POLL_FUNC, serverGetLastError());
		#endif

		return(-1);
	}else{
		// Check whether the master socket is ready to accept new connections.
		if(flagsAreSet(socketHandlerMasterHandle(handler)->revents, POLLIN)){
			socketInfo newInfo;
			socketHandle newHandle;

			newInfo.addressSize = sizeof(struct sockaddr);
			newHandle.fd = accept(socketHandlerMasterHandle(handler)->fd, (struct sockaddr *)&newInfo.address, &newInfo.addressSize);

			if(newHandle.fd == INVALID_SOCKET){
				#ifdef SERVER_DEBUG
				serverPrintError("accept()", serverGetLastError());
				#endif

			// If the connection was accepted successfully,
			// add the new socket to our connection handler!
			}else{
				const return_t success = (
					newHandle.events  = POLLIN,
					newHandle.revents = 0,
					socketHandlerAdd(handler, &newHandle, &newInfo)
				);
				// The connection handler is full or could not be resized.
				if(success <= 0){
					#ifdef SERVER_DEBUG
					puts("Warning: Incoming connection rejected - connection handler is full.\n");
					#endif
					socketclose(newHandle.fd);
				}
			}

			socketHandlerMasterHandle(handler)->revents = 0;
			return(changedSockets - 1);
		}
	}

	return(changedSockets);
}

/*
** If the socket has sent data, store it in "buffer" and return the size of the buffer.
** Otherwise, return 0 if they have disconnected and -1 if they were idle.
*/
int serverReceiveTCP(socketInfo *const restrict client, char *const restrict buffer){
	const socketHandle curHandle = *client->handle;
	client->handle->revents = 0;

	// The socket has changed state.
	if(curHandle.revents != 0){
		// The socket has disconnected.
		if(flagsAreSet(curHandle.revents, POLLHUP)){
			return(0);

		// The socket has sent some data.
		}else if(flagsAreSet(curHandle.revents, POLLIN)){
			const int bufferLength = recv(curHandle.fd, buffer, SERVER_MAX_BUFFER_SIZE, 0);
			// Error sending data.
			if(bufferLength < 0){
				#ifdef SERVER_DEBUG
				serverPrintError("recv()", serverGetLastError());
				#endif
			}

			return(bufferLength);
		}
	}

	// If the socket was idle, return -1. This means that if there was
	// an error receiving data from the client, they are considered idle.
	return(-1);
}

// Send data to a socket.
return_t serverSendTCP(const socketServer *const restrict handler, const socketInfo *const restrict client, const char *const restrict buffer, const size_t bufferLength){
	if(send(client->handle->fd, buffer, bufferLength, 0) < 0){
		#ifdef SERVER_DEBUG
		serverPrintError("send()", serverGetLastError());
		#endif

		return(0);
	}

	return(1);
}

// Disconnect a socket.
void serverDisconnectTCP(socketServer *const restrict handler, socketInfo *const restrict client){
	socketclose(client->handle->fd);
	socketHandlerRemove(handler, client);
}

// Shutdown the server.
void serverCloseTCP(socketServer *const restrict handler){
	socketInfo *curInfo = &handler->info[1];
	const socketInfo *const lastInfo = &handler->info[handler->nfds];

	// Disconnect all of the clients.
	for(; curInfo < lastInfo; ++curInfo){
		serverDisconnectTCP(handler, curInfo);
	}
	// Close the master socket.
	socketclose(socketHandlerMasterHandle(handler)->fd);

	serverClose(handler);
}


#if 0
// Poll each socket and update their buffers and flags.
return_t serverListenTCP(socketHandler *const restrict handler){
	int changedSockets = pollFunc(handler->handles, handler->nfds, SERVER_POLL_TIMEOUT);
	// Make sure there wasn't an error while polling.
	if(changedSockets == SOCKET_ERROR){
		#ifdef SERVER_DEBUG
		serverPrintError(SERVER_POLL_FUNC, serverGetLastError());
		#endif
		return(0);
	}else{
		socketInfo *curInfo = &handler->info[1];
		size_t remainingSockets = handler->nfds - 1;

		// Check whether the master socket is ready to accept new connections.
		if(flagsAreSet(socketHandlerMasterHandle(handler)->fd, POLLIN)){
			socketInfo newInfo;
			socketHandle newHandle;

			newInfo.addressSize = sizeof(struct sockaddr);
			newHandle.fd = accept(socketHandlerMasterHandle(handler)->fd, (struct sockaddr *)&newInfo.address, &newInfo.addressSize);

			if(newHandle.fd == INVALID_SOCKET){
				#ifdef SERVER_DEBUG
				serverPrintError("accept()", serverGetLastError());
				#endif

			// If the connection was accepted successfully,
			// add the new socket to our connection handler!
			}else{
				const return_t success = (
					newInfo.flags     = SERVER_SOCKET_INFO_CONNECTED,
					newHandle.events  = POLLIN | POLLHUP,
					newHandle.revents = 0,
					socketHandlerAdd(handler, &newHandle, &newInfo)
				);
				// The connection handler is full or could not be resized.
				if(success <= 0){
					#ifdef SERVER_DEBUG
					puts("Warning: Incoming connection rejected - connection handler is full.\n");
					#endif
					socketclose(newHandle.fd);
				}
			}

			socketHandlerMasterHandle(handler)->revents = 0;
			--changedSockets;
		}

		// Check each socket's flags to see if they've changed state.
		while(remainingSockets > 0){
			if(socketInfoValid(curInfo)){
				const socketHandle curHandle = *curInfo->handle;

				// The socket has changed state.
				if(curHandle.revents != 0){
					// The socket has disconnected.
					if(flagsAreSet(curHandle.revents, POLLHUP)){
						flagsSet(curInfo->flags, SERVER_SOCKET_INFO_DISCONNECTED);

					// The socket has sent some data.
					}else if(flagsAreSet(curHandle.revents, POLLIN)){
						curInfo->lastBufferLength = recv(curHandle.fd, curInfo->lastBuffer, SERVER_MAX_BUFFER_SIZE, 0);

						switch(curInfo->lastBufferLength){
							// Error sending data.
							case -1:
								#ifdef SERVER_DEBUG
								serverPrintError("recv()", serverGetLastError());
								#endif
								flagsSet(curInfo->flags, SERVER_SOCKET_INFO_ERROR);
							break;

							// Socket disconnected gracefully.
							case 0:
								flagsSet(curInfo->flags, SERVER_SOCKET_INFO_DISCONNECTED);
							break;

							// Data received successfully.
							default:
								flagsSet(curInfo->flags, SERVER_SOCKET_INFO_SENT_DATA);
						}
					}

					curInfo->handle->revents = 0;
					--changedSockets;
				}

				--remainingSockets;
			}

			++curInfo;
		}
	}

	return(1);
}
#endif