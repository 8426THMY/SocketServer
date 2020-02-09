#include "socketHandler.h"



// If the size of a socket handle is not the same as that of a pollfd, we
// should trigger a compile-time error, as our polling functions won't work.
#define COMPILE_TIME_ERROR(condition) ((void)sizeof(char[1 - 2*!!(condition)]));


// Forward-declare any helper functions!
#ifdef SERVER_SOCKET_HANDLER_REALLOCATE
static return_t socketHandlerResize(socketHandler *handler);
#endif


return_t socketHandlerInit(socketHandler *handler, const size_t capacity, const socketHandle *masterHandle, const socketInfo *masterInfo){
	socketHandle *handle;
	const socketInfo *lastInfo;
	size_t id;

	// This line checks a compile-time condition and should generate no code.
	COMPILE_TIME_ERROR(sizeof(socketInfo *) > sizeof(socketHandle));

	// Allocate enough memory for the maximum number of connected sockets.
	socketInfo *info = malloc(capacity * (sizeof(socketInfo) + sizeof(socketHandle)));
	if(info == NULL){
		return(-1);
	}
	handler->info     = info;
	handler->lastInfo = info - 1;

	// Initialize the socket handle array.
	handle = (socketHandle *)&info[capacity];
	handler->handles    = handle;
	handler->lastHandle = handle - 1;

	// Initialize the socket handles and information.
	lastInfo = (socketInfo *)handle;
	for(id = 0; info < lastInfo; ++handle, ++info, ++id){
		*((socketInfo **)handle) = info;
		info->handle = NULL;
		info->id = id;
	}

	handler->capacity = capacity;
	handler->nfds = 0;

	// Add the master socket to the connection handler.
	return(socketHandlerAdd(handler, masterHandle, masterInfo));
}


// Add a new socket to a socket handler!
return_t socketHandlerAdd(socketHandler *handler, const socketHandle *handle, const socketInfo *info){
	if(handler->nfds >= handler->capacity){
		#ifdef SERVER_SOCKET_HANDLER_REALLOCATE
			if(socketHandlerResize(handler) < 0){
				return(-1);
			}
		#else
			return(0);
		#endif
	}

	{
		socketHandle *newHandle = ++handler->lastHandle;
		socketInfo *newInfo = *((socketInfo **)newHandle);

		*newHandle = *handle;
		newInfo->handle           = newHandle;
		newInfo->address          = info->address;
		newInfo->addressSize      = info->addressSize;

		handler->lastInfo = newInfo;
		++handler->nfds;
	}

	return(1);
}

// Remove a socket from a socket handler!
return_t socketHandlerRemove(socketHandler *handler, socketInfo *info){
	// Make sure we don't remove the master socket.
	if(info == socketHandlerMasterInfo(handler)){
		return(0);
	}

	// Move the last socket information's handle over
	// to fill the gap left by the one we're removing.
	*info->handle = *handler->lastHandle;
	handler->lastInfo->handle = info->handle;
	// The now-unused last handle should now store
	// a pointer to this socket's information.
	info->handle = NULL;
	*((socketInfo **)handler->lastHandle) = info;

	--handler->lastHandle;
	--handler->nfds;


	return(1);
}


void socketHandlerDelete(socketHandler *handler){
	free(handler->info);
}


#ifdef SERVER_SOCKET_HANDLER_REALLOCATE
static return_t socketHandlerResize(socketHandler *handler){
	uintptr_t handleOffset;
	socketHandle *handle;
	socketHandle *oldHandle;
	const socketInfo *lastInfo;

	size_t remainingHandles = socketHandler->nfds;
	size_t remainingInfo    = remainingHandles;

	size_t capacity = handler->capacity << 1;
	// Resize the connection handler's array.
	socketInfo *info = realloc(handler->info, capacity * (sizeof(socketInfo) + sizeof(socketHandle)));
	if(buffer == NULL){
		return(-1);
	}
	handle    = (socketHandle *)&info[capacity];
	oldHandle = (socketHandle *)&info[handler->capacity];
	socketHandler->capacity = capacity;

	// Negative offsets should work despite the overflow.
	handleOffset = ((uintptr_t)handle) - ((uintptr_t)sc->handles);
	// Move the array pointers using the offsets from the old arrays to the new one.
	sc->info = info;
	sc->lastInfo = (socketInfo *)(((uintptr_t)sc->lastInfo) + ((uintptr_t)info) - ((uintptr_t)sc->info));
	sc->handles = handle;
	sc->lastHandle = (socketHandle *)(((uintptr_t)sc->lastHandle) + handleOffset);

	// Use capacity as an ID iterator for new socket information objects.
	capacity -= 1;
	for(; info < lastInfo; ++info, ++handle){
		// Copy the file descriptors that we're using.
		if(remainingHandles > 0){
			*handle = *oldHandle;
			++oldHandle;
			--remainingHandles;

		// Unused handles should point to the
		// corresponding socket information.
		}else{
			*((socketInfo **)handle) = info;
		}

		if(remainingInfo > 0){
			// We need to adjust the pointers for our
			// existing socket information handles.
			if(socketInfoValid(info)){
				info->handle = (socketHandle *)(((uintptr_t)info->handle) + handleOffset);
				--remainingInfo;
			}

		// Initialize the new socket information objects.
		}else{
			info->handle = NULL;
			info->id = capacity;
			++capacity;
		}
	}

	return(1);
}
#endif