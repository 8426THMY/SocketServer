#ifndef socketHandler_h
#define socketHandler_h


#include <stdlib.h>
#include <stdint.h>

#include "socketShared.h"
#include "../utilTypes.h"


#define socketInfoValid(info) ((info)->handle != NULL)
#define socketHandlerMasterHandle(handler) ((handler)->handles)
#define socketHandlerMasterInfo(handler)   ((handler)->info)


typedef struct socketInfo {
	// Pointer to the handle described by this socket information.
	// If this object is inactive, this pointer should be NULL.
	socketHandle *handle;

	// This is the socket information's position in the connection handler's array.
	size_t id;
	struct sockaddr_storage address;
	socketAddressLength addressSize;
} socketInfo;

typedef struct socketHandler {
	// Array of socket information, one for each socket handle.
	socketInfo *info;
	// This points to the last socket information we added, which
	// is not necessarily the last socket information in our array.
	socketInfo *lastInfo;

	// Array of pollfds used for TCP polling.
	socketHandle *handles;
	// We always use the last handle in the array when adding a new socket.
	// Unused handles store pointers to the socket information they used last.
	socketHandle *lastHandle;

	// Total number of allocated file descriptors.
	size_t capacity;
	// Total number of used file descriptors used for TCP polling.
	size_t nfds;
} socketHandler;


return_t socketHandlerInit(socketHandler *handler, const size_t capacity, const socketHandle *masterHandle, const socketInfo *masterInfo);

return_t socketHandlerAdd(socketHandler *handler, const socketHandle *handle, const socketInfo *info);
return_t socketHandlerRemove(socketHandler *handler, socketInfo *info);

void socketHandlerDelete(socketHandler *handler);


#endif