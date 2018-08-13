#ifndef socketHandler_h
#define socketHandler_h


#include <stdlib.h>

#include "socketShared.h"


typedef struct socketInfo {
	size_t id;

	struct sockaddr_storage addr;
	int addrSize;

	//Array containing the data received from the client this update.
	char **bufferArray;
	size_t bufferArraySize;
	size_t bufferArrayCapacity;

	unsigned long lastUpdateTick;
} socketInfo;

typedef struct socketHandler {
	struct pollfd *fds;
	socketInfo *info;
	//Stack containing unused client I.D.s.
	size_t *idStack;
	//Stores which fd each client I.D. is linked to.
	size_t *idLinks;

	size_t capacity;
	size_t size;
} socketHandler;


void socketInfoInit(socketInfo *info, socketHandler *handler);
unsigned char handlerInit(socketHandler *handler, size_t capacity, struct pollfd *fd, socketInfo *info);

unsigned char socketInfoBufferResize(socketInfo *info, size_t capacity);
void socketInfoBufferAdd(socketInfo *info, char *buffer, size_t bufferLength);

unsigned char handlerResize(socketHandler *handler, size_t capacity);
unsigned char handlerAdd(socketHandler *handler, struct pollfd *fd, socketInfo *info);
unsigned char handlerRemove(socketHandler *handler, size_t pos);

void socketInfoBufferClear(socketInfo *info);
void handlerClear(socketHandler *handler);


#endif