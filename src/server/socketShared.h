#ifndef socketShared_h
#define socketShared_h


#include "../serverSettings.h"


// Number of users plus one for the master socket.
#ifndef SERVER_MAX_SOCKETS
	#define SERVER_MAX_SOCKETS 201
#endif
#ifndef SERVER_USE_POLL
	#define SERVER_POLL_FUNC "select()"
#else
	#define SERVER_POLL_FUNC "poll()"
#endif
#ifndef SERVER_POLL_TIMEOUT
	#define SERVER_POLL_TIMEOUT 0
#endif
// Number of milliseconds to wait before disconnecting an inactive socket.
#ifndef SERVER_SOCKET_TIMEOUT
	#define SERVER_SOCKET_TIMEOUT 30000
#endif
#ifndef SERVER_DEFAULT_ADDRESS_FAMILY
    #define SERVER_DEFAULT_ADDRESS_FAMILY AF_INET
#endif
#ifndef SERVER_DEFAULT_PORT
    #define SERVER_DEFAULT_PORT 8426
#endif

#undef FD_SETSIZE
#undef __FD_SETSIZE
#define FD_SETSIZE SERVER_MAX_SOCKETS
#define __FD_SETSIZE SERVER_MAX_SOCKETS


#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
	#include <ws2tcpip.h>

	#define WS_VERSION MAKEWORD(2, 2)

	#define socketclose(x) closesocket(x)

	// Regular data can be received.
	#ifndef POLLIN
		#define POLLIN     0x100
	#endif
	// Priority data can be received.
	#ifndef POLLPRI
		#define POLLPRI    0x200
	#endif
	// Regular data can be sent.
	#ifndef POLLOUT
		#define POLLOUT    0x010
	#endif
	// The socket has hung up.
	#ifndef POLLHUP
		#define POLLHUP    0x002
	#endif

	#ifndef EWOULDBLOCK
		#define EWOULDBLOCK WSAEWOULDBLOCK
	#endif
	#ifndef ECONNRESET
		#define ECONNRESET WSAECONNRESET
	#endif


	struct pollfd {
		int fd;
		short events;
		short revents;
	};

	#define socketHandle struct pollfd
	#define sockAddrLen_t int


	int inet_pton(int af, const char *src, char *dst);
	const char *inet_ntop(int af, const void *src, char *dst, size_t size);
#else
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <errno.h>

	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1

	#define socketclose(x) close(x)

	#ifdef SERVER_USE_POLL
		#include <sys/poll.h>
	#else
		// Regular data can be received (checks for POLLRDNORM and POLLRDBAND).
		#ifndef POLLIN
			#define POLLIN     0x001
		#endif
		// Priority data can be received.
		#ifndef POLLPRI
			#define POLLPRI    0x002
		#endif
		// Data can be sent (checks for POLLWRNORM).
		#ifndef POLLOUT
			#define POLLOUT    0x004
		#endif
		// An error occurred.
		#ifndef POLLERR
			#define POLLERR    0x008
		#endif
		// The socket has hung up.
		#ifndef POLLHUP
			#define POLLHUP    0x010
		#endif
		// Invalid socket descriptor.
		#ifndef POLLNVAL
			#define POLLNVAL   0x020
		#endif
		// Regular data can be read.
		#ifndef POLLRDNORM
			#define POLLRDNORM 0x040
		#endif
		// Out-of-band data can be read.
		#ifndef POLLRDBAND
			#define POLLRDBAND 0x080
		#endif
		// Regular data can be sent.
		#ifndef POLLWRNORM
			#define POLLWRNORM 0x100
		#endif
		// Out-of-band data can be sent.
		#ifndef POLLWRBAND
			#define POLLWRBAND 0x200
		#endif


		struct pollfd {
			int fd;
			short events;
			short revents;
		};

		#define socketHandle struct pollfd
		#define sockAddrLen_t socklen_t
	#endif
#endif

#ifdef SERVER_DEBUG
#include <stdio.h>
#endif


int pollFunc(struct pollfd *fdarray, size_t nfds, int timeout);

#ifdef SERVER_DEBUG
int serverGetLastError();
void serverPrintError(const char *func, const int code);
#endif


#endif