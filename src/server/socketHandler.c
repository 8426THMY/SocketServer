#include "socketHandler.h"


void socketInfoInit(socketInfo *info, socketHandler *handler){
	//Assign them a unique I.D.!
	info->id = handler->idStack[handler->size];
	//If this isn't the master socket, initialize the buffer array!
	if(info->id != 0){
		info->bufferArray = malloc(sizeof(*info->bufferArray));
		info->bufferArraySize = 0;
		info->bufferArrayCapacity = 1;

		//info->lastUpdateTick = currentTick;
	}else{
		info->bufferArray = NULL;
		info->bufferArraySize = 0;
		info->bufferArrayCapacity = 0;
	}

	//Add the new socketInfo to our handler!
	handler->info[handler->size] = *info;

	//Create a link for the new I.D. and remove it from the stack!
	handler->idLinks[handler->idStack[handler->size]] = handler->size;
	handler->idStack[handler->size] = 0;
	++handler->size;
}

unsigned char handlerInit(socketHandler *handler, size_t capacity, struct pollfd *fd, socketInfo *info){
	if(capacity > 0){
		handler->fds = NULL;
		handler->info = NULL;
		handler->idStack = NULL;
		handler->idLinks = NULL;

		handler->capacity = 0;
		handler->size = 0;

		return(handlerResize(handler, capacity) && handlerAdd(handler, fd, info));
	}

	return(0);
}


unsigned char socketInfoBufferResize(socketInfo *info, size_t capacity){
	char **tempData = realloc(info->bufferArray, sizeof(*info->bufferArray) * capacity);
	if(tempData != NULL){
		info->bufferArray = tempData;
		info->bufferArrayCapacity = capacity;

		return(1);
	}


	return(0);
}

//Add a message to a buffer array!
void socketInfoBufferAdd(socketInfo *info, char *buffer, size_t bufferLength){
	if(info->bufferArraySize == info->bufferArrayCapacity){
		socketInfoBufferResize(info, info->bufferArrayCapacity * 2);
	}
	info->bufferArray[info->bufferArraySize] = malloc(bufferLength);
	memcpy(info->bufferArray[info->bufferArraySize], buffer, bufferLength);
	++info->bufferArraySize;
}


unsigned char handlerResize(socketHandler *handler, size_t capacity){
	if(capacity != handler->capacity){
		//Resize the pollfd array!
		void *tempBuffer = realloc(handler->fds, capacity * sizeof(*handler->fds));
		if(tempBuffer != NULL){
			handler->fds = tempBuffer;
		}else{
			return(0);
		}
		//Resize the socketInfo array!
		tempBuffer = realloc(handler->info, capacity * sizeof(*handler->info));
		if(tempBuffer != NULL){
			handler->info = tempBuffer;
		}else{
			free(handler->fds);

			return(0);
		}
		//Resize the idStack!
		tempBuffer = realloc(handler->idStack, capacity * sizeof(*handler->idStack));
		if(tempBuffer != NULL){
			handler->idStack = tempBuffer;
		}else{
			free(handler->fds);
			free(handler->info);

			return(0);
		}
		//Resize the idLinks array!
		tempBuffer = realloc(handler->idLinks, capacity * sizeof(*handler->idLinks));
		if(tempBuffer != NULL){
			handler->idLinks = tempBuffer;
		}else{
			free(handler->fds);
			free(handler->info);
			free(handler->idStack);

			return(0);
		}

		//Fill the I.D. stack!
		size_t i;
		for(i = handler->capacity; i < capacity; ++i){
			handler->idStack[i] = i;
			handler->idLinks[i] = 0;
		}

		handler->capacity = capacity;


		return(1);
	}


	return(0);
}

unsigned char handlerAdd(socketHandler *handler, struct pollfd *fd, socketInfo *info){
	if(fd != NULL || info != NULL){
		//Add the user's details to the fd array!
		if(fd != NULL){
			handler->fds[handler->size] = *fd;
		}
		//Add the user's details to the socketInfo array!
		if(info != NULL){
			socketInfoInit(info, handler);
		}


		return(1);
	}


	return(0);
}

unsigned char handlerRemove(socketHandler *handler, size_t socketID){
	if(handler != NULL && socketID < handler->capacity && handler->idLinks[socketID] != 0){
		--handler->size;

		size_t i = handler->idLinks[socketID];
		//Make sure we clear this socket's buffer array.
		socketInfoBufferClear(&handler->info[i]);
		free(&handler->info[i].bufferArray);

		//Now shift everything over!
		while(i < handler->size){
			handler->fds[i] = handler->fds[i + 1];
			handler->info[i] = handler->info[i + 1];
			--handler->idLinks[handler->info[i].id];

			++i;
		}

		//Put the old user's I.D. at the front of the stack and clear the link.
		handler->idStack[handler->size] = socketID;
		handler->idLinks[socketID] = 0;


		return(1);
	}


	return(0);
}


//Clear a socket's buffer array.
//We don't free the actual array variable because we want to keep its current size.
void socketInfoBufferClear(socketInfo *info){
	size_t i;
	for(i = 0; i < info->bufferArraySize; ++i){
		free(info->bufferArray[i]);
	}
	info->bufferArraySize = 0;
}

void handlerClear(socketHandler *handler){
	if(handler->fds != NULL){
		free(handler->fds);
	}
	if(handler->info != NULL){
		size_t i;
		for(i = 0; i < handler->size; ++i){
			socketInfoBufferClear(&handler->info[i]);
			free(&handler->info[i].bufferArray);
		}
		free(handler->info);
	}
	if(handler->idStack != NULL){
		free(handler->idStack);
	}
	if(handler->idLinks != NULL){
		free(handler->idLinks);
	}
}