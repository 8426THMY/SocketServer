#ifndef serverSettings_h
#define serverSettings_h


#define SERVER_MAX_SOCKETS 201
// Limits the amount of information that
// can be received by a socket at once.
#define SERVER_MAX_BUFFER_SIZE 2048

#define SERVER_USE_POLL
// Note: This should be variable, and preferably an input to "socketListen".
// If no one is sending anything, it should block until it's time to update.
// If a negative timeout is specified, we should block indefinitely.
#define SERVER_POLL_TIMEOUT_SEC  0
#define SERVER_POLL_TIMEOUT_USEC 0
// The number of seconds and microseconds are added to get the total timeout.
#define SERVER_POLL_TIMEOUT ((SERVER_POLL_TIMEOUT_SEC) * 1000 + (SERVER_POLL_TIMEOUT_USEC) / 1000)

#define SERVER_DEBUG


#endif