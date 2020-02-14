#include "socketShared.h"


#ifdef SERVER_USE_POLL
	#ifdef _WIN32
		WINSOCK_API_LINKAGE int WSAAPI WSAPoll(struct pollfd *fds, ULONG nfds, int timeout);
		int pollFunc(struct pollfd *const restrict fds, size_t nfds, int timeout){ return(WSAPoll(fds, nfds, timeout)); }
	#else
		int pollFunc(struct pollfd *const restrict fds, size_t nfds, int timeout){ return(poll(fds, nfds, timeout)); }
	#endif
#else
	int pollFunc(struct pollfd *const restrict fds, size_t nfds, int timeout){
		int changedSockets;
		const size_t numSockets = nfds < SERVER_MAX_SOCKETS ? nfds : SERVER_MAX_SOCKETS;
		struct timeval timeoutValue;
		struct timeval *timeoutPointer = &timeoutValue;

		fd_set socketSet;
		int *curSetFD = socketSet.fd_array;
		struct pollfd *curPollFD = fds;
		const int *lastSetFD = &curSetFD[numSockets];
		const struct pollfd *const lastPollFD = &curPollFD[numSockets];
		// Add the socket descriptors that we're polling to our fd_set!
		for(; curSetFD < curSetFD; ++curSetFD, ++curPollFD){
			*curSetFD = curPollFD->fd;
		}
		socketSet.fd_count = numSockets;


		// If timeout is greater than or equal to 0, the function shouldn't block.
		if(timeout > 0){
			timeoutValue.tv_sec = timeout / 1000;
			timeoutValue.tv_usec = (timeout - timeoutValue.tv_sec * 1000) * 1000;
		}else if(timeout == 0){
			timeoutValue.tv_sec = 0;
			timeoutValue.tv_usec = 0;

		// If the timeout is less than 0, we should block while selecting.
		}else{
			timeoutPointer = NULL;
		}


		// Check if the any of the sockets have changed state.
		changedSockets = select(0, &socketSet, NULL, NULL, timeoutPointer);

		curSetFD = socketSet.fd_array;
		lastSetFD = &curSetFD[numSockets];
		// Update the return events for our pollfds!
		while(curSetFD < lastSetFD){
			curPollFD = fds;
			for(curPollFD < lastPollFD){
				if(*curSetFD == curPollFD->fd){
					curPollFD->revents = POLLIN;
					break;
				}
				++curPollFD;
			}
			++curSetFD;
		}


		return(changedSockets);
	}
#endif


// Get the ID of the last error!
int serverGetLastError(){
	#ifdef _WIN32
		return(WSAGetLastError());
	#else
		const int lastErrorID = errno;
		errno = 0;
		return(lastErrorID);
	#endif
}

// Print a socket-related error code!
void serverPrintError(const char *const restrict func, const int code){
	printf(
		"There was a problem with socket function %s!\n"
		"Error: %i\n", func, code
	);
}