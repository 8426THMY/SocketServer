#ifndef socketServer_h
#define socketServer_h


#include <stddef.h>
#include <stdint.h>

#include "socketShared.h"
#include "../utilTypes.h"


#define socketInfoValid(info) ((info)->handle != NULL)
#define serverGetNextSocket(info) for(; !socketInfoValid(info); ++info)

#define socketHandlerMasterHandle(handler) ((handler)->handles)
#define socketHandlerMasterInfo(handler)   ((handler)->info)


typedef struct socketServer socketServer;
typedef struct socketServerConfig {
	int type;
	int protocol;
	char ip[46];
	uint16_t port;
} socketServerConfig;


typedef struct socketInfo {
	// Pointer to the handle described by this socket information.
	// If this object is inactive, this pointer should be NULL.
	socketHandle *handle;

	// This is the socket information's position in the connection handler's array.
	size_t id;
	struct sockaddr_storage address;
	sockAddrLen_t addressSize;
} socketInfo;

typedef struct socketServer {
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
} socketServer;


void serverConfigInit(socketServerConfig *cfg, const int type, const int protocol);

#ifdef _WIN32
return_t serverSetup();
void serverCleanup();
#else
#define serverSetup() 1
#define serverCleanup() ;
#endif
return_t serverInit(socketServer *server, socketServerConfig cfg);
#define serverClose(server) socketHandlerDelete(server)

return_t socketHandlerInit(socketServer *handler, const size_t capacity, const socketHandle *masterHandle, const socketInfo *masterInfo);
return_t socketHandlerAdd(socketServer *handler, const socketHandle *handle, const socketInfo *info);
return_t socketHandlerRemove(socketServer *handler, socketInfo *info);
void socketHandlerDelete(socketServer *handler);


#endif