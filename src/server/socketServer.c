#include "socketServer.h"


#include <stdlib.h>
#include <string.h>

#include "socketShared.h"


// If the size of a socket handle is not the same as that of a pollfd, we
// should trigger a compile-time error, as our polling functions won't work.
#define COMPILE_TIME_ERROR(condition) ((void)sizeof(char[1 - 2*!!(condition)]));


// Forward-declare any helper functions!
static int getAddressFamily(const char *ip);
static return_t setNonBlockMode(const int fd, unsigned long mode);
#ifdef SERVER_SOCKET_HANDLER_REALLOCATE
static return_t socketHandlerResize(socketServer *handler);
#endif


void serverConfigInit(socketServerConfig *cfg, const int type, const int protocol){
	cfg->type = type;
	cfg->protocol = protocol;
	cfg->ip[0] = '\0';
	cfg->port = SERVER_DEFAULT_PORT;
}


#ifdef _WIN32
// Initialize the Winsock API.
return_t serverSetup(){
	WSADATA wsaData;
	int wsaError = WSAStartup(WS_VERSION, &wsaData);
	if(wsaError != 0){
		#ifdef SERVER_DEBUG
		serverPrintError("WSAStartup()", serverGetLastError());
		#endif

		return(0);
	}

	return(1);
}

// Cleanup the Winsock API.
void serverCleanup(){
	WSACleanup();
}
#endif

return_t serverInit(socketServer *server, socketServerConfig cfg){
	int af;
	socketHandle masterHandle;
	socketInfo masterInfo;


	// Check the address family that the specified IP is using.
	af = getAddressFamily(cfg.ip);

	// Create a TCP/UDP socket that can be bound to IPv4 or IPv6 addresses.
	masterHandle.fd = socket(af, cfg.type, cfg.protocol);
	if(masterHandle.fd == INVALID_SOCKET){
		#ifdef SERVER_DEBUG
		serverPrintError("socket()", serverGetLastError());
		#endif

		return(0);
	}
	masterHandle.events = POLLIN;
	masterHandle.revents = 0;


	// If SERVER_POLL_TIMEOUT is not equal to 0, set the timeout for "recvfrom()".
	if(SERVER_POLL_TIMEOUT != 0){
		#ifdef _WIN32
		const long timeout = SERVER_POLL_TIMEOUT;
		#else
		const struct timeval timeout = {
			.tv_sec  = SERVER_POLL_TIMEOUT_SEC,
			.tv_usec = SERVER_POLL_TIMEOUT_USEC
		};
		#endif
		if(setsockopt(masterHandle.fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) == SOCKET_ERROR){
			#ifdef SERVER_DEBUG
			serverPrintError("setsockopt()", serverGetLastError());
			#endif

			return(0);
		}
	}else{
		setNonBlockMode(masterHandle.fd, 1);
	}


	// Store the host's address depending on its IP version.
	if(af == AF_INET){
		struct sockaddr_in hostAddr4;
		if(!inet_pton(af, cfg.ip, (char *)&(hostAddr4.sin_addr))){
			hostAddr4.sin_addr.s_addr = INADDR_ANY;
			strcpy(cfg.ip, "INADDR_ANY");
		}
		hostAddr4.sin_family = af;
		hostAddr4.sin_port = htons(cfg.port);

		masterInfo.address = *((struct sockaddr_storage *)&hostAddr4);
		masterInfo.addressSize = sizeof(struct sockaddr_in);
	}else if(af == AF_INET6){
		struct sockaddr_in6 hostAddr6;
		memset(&hostAddr6, 0, sizeof(struct sockaddr_in6));
		if(!inet_pton(af, cfg.ip, (char *)&(hostAddr6.sin6_addr))){
			hostAddr6.sin6_addr = in6addr_any;
			strcpy(cfg.ip, "in6addr_any");
		}
		hostAddr6.sin6_family = af;
		hostAddr6.sin6_port = htons(cfg.port);

		masterInfo.address = *((struct sockaddr_storage *)&hostAddr6);
		masterInfo.addressSize = sizeof(struct sockaddr_in6);
	}


	// Bind the master socket to the host's address!
	if(bind(masterHandle.fd, (struct sockaddr *)&masterInfo.address, sizeof(struct sockaddr_storage)) == SOCKET_ERROR){
		#ifdef SERVER_DEBUG
		serverPrintError("bind()", serverGetLastError());
		#endif

		return(0);
	}

	// Start listening for connections if the socket is using TCP!
	if(cfg.protocol == IPPROTO_TCP){
		if(listen(masterHandle.fd, SOMAXCONN) == SOCKET_ERROR){
			#ifdef SERVER_DEBUG
			serverPrintError("listen()", serverGetLastError());
			#endif

			return(0);
		}
	}

	// Initialize our connection handler and push the master socket into it!
	if(!socketHandlerInit(server, SERVER_MAX_SOCKETS, &masterHandle, &masterInfo)){
		#ifdef SERVER_DEBUG
		puts("Error: Failed to initialize connection handler.\n");
		#endif

		return(0);
	}


	#ifdef SERVER_DEBUG
	printf(
		"Server initialized successfully.\n\n"
		"Listening on %s:%u...\n\n\n", cfg.ip, cfg.port
	);
	#endif


	return(1);
}


return_t socketHandlerInit(socketServer *handler, const size_t capacity, const socketHandle *masterHandle, const socketInfo *masterInfo){
	socketHandle *handle;
	const socketInfo *lastInfo;
	size_t id = 0;

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
	for(; info < lastInfo; ++handle, ++info, ++id){
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
return_t socketHandlerAdd(socketServer *handler, const socketHandle *handle, const socketInfo *info){
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
return_t socketHandlerRemove(socketServer *handler, socketInfo *info){
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

void socketHandlerDelete(socketServer *handler){
	free(handler->info);
}


static int getAddressFamily(const char *ip){
	char buffer[16];
	if(inet_pton(AF_INET, ip, buffer)){
		return(AF_INET);
	}else if(inet_pton(AF_INET6, ip, buffer)){
		return(AF_INET6);
	}

	return(SERVER_DEFAULT_ADDRESS_FAMILY);
}

// If mode is 0, make socket functions block. Otherwise, make them non-blocking.
static return_t setNonBlockMode(const int fd, unsigned long mode){
	#ifdef _WIN32
		return(!ioctlsocket(fd, FIONBIO, &mode));
	#else
		int flags = fcntl(fd, F_GETFL, 0);
		if(flags < 0){
			return(0);
		}

		flags = mode ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
		return(!fcntl(fd, F_SETFL, flags));
	#endif
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