#include "socketServer.h"


#include <stdlib.h>
#include <string.h>

#include "socketShared.h"


// Forward-declare any helper functions!
static int getAddressFamily(const char *ip);
static return_t setNonBlockMode(const int fd, unsigned long mode);


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
#endif



void serverConfigInit(socketServerConfig *cfg, const int type, const int protocol){
	cfg->type = type;
	cfg->protocol = protocol;
	cfg->ip[0] = '\0';
	cfg->port = SERVER_DEFAULT_PORT;
}

return_t serverInit(socketServer *server, socketServerConfig cfg){
	int af;
	socketHandle masterHandle;
	socketInfo masterInfo;


	server->type = cfg.type;
	server->protocol = cfg.protocol;


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
	if(!socketHandlerInit(&server->connectionHandler, SERVER_MAX_SOCKETS, &masterHandle, &masterInfo)){
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


#ifdef _WIN32
// Cleanup the Winsock API.
void serverCleanup(){
	WSACleanup();
}
#endif


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