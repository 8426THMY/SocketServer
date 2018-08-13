#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#include "server/socketShared.h"
#include "server/socketUDP.h"


/** UDP client-finding should be optional. I came up with a better solution to it: **/
/* When a client sends the connect message, the server assigns them a unique ID.     */
/* All future packets that aresent by the client includes this I.D. When a packet is */
/* receieved, the server uses the ID as the index to an array of connected client    */
/* addresses. If this client's address is the same as the one in the array, handle   */
/* what they've sent! Otherwise, it's probably an imposter, so disconnect them. If   */
/* you want it to work with the current system, you might have to make it store the  */
/* buffers for each user under the master socket's socketInfo thing.                 */

/** Also, this doesn't work very well if you want a dumb client. **/
/* When data is received in the listen function, it is added to an array of data   */
/* received from that client. Instead of calling a function to handle it straight  */
/* away, handle it inside an update function that the developer defines. Ideally,  */
/* this function would use each user's buffers to create an input state, then call */
/* another function to update the global game state.                               */

/** We shouldn't handle client timeouts. That should be left to the developer. **/

/** How should we handle disconnections? **/
/* We will probably need to store whether or not each client is currently      */
/* connected and let the developer choose what to do with them if they aren't. */
/* Either that or have an array that stores the I.D.s of clients who have      */
/* disconnected. It would likely  have to be flushed after each update like    */
/* the clients' buffer arrays.                                                 */

/** You still need a function for handling stuff as soon as it's received. **/
/* The problem is, how do you take inputs into it? */


//Forward-declare our helper functions!
static void bufferFunc(socketServer *server, const socketInfo *client);
static void disconnectFunc(socketServer *server, const socketInfo *client);

static unsigned char loadServer(socketServer *server, const int type, const int protocol,
                                const char *configPath, void (*configFunc)(const char *configPath, char **ip, size_t *ipLength, unsigned short *port, size_t *bufferSize),
                                void (*discFunc)(socketServer *server, const socketInfo *client));
static void loadConfig(const char *configPath, char **ip, size_t *ipLength, unsigned short *port, size_t *bufferSize);
static char *readLineFile(FILE *file, char *line, size_t *lineLength);


int main(int argc, char *argv[]){
	if(serverSetup()){
		socketServer server;

		if(loadServer(&server, SOCK_DGRAM, IPPROTO_UDP, "./config/config.cfg", &loadConfig, &disconnectFunc)){
			unsigned char running = 1;
			while(running){
				running = serverListenUDP(&server);

				size_t i;
				for(i = 0; i < server.connectionHandler.size; ++i){
					bufferFunc(&server, &server.connectionHandler.info[i]);
					socketInfoBufferClear(&server.connectionHandler.info[i]);
				}
			}

			serverCloseUDP(&server);
		}
    }
    serverCleanup();


	puts("\n\nPress enter to exit.");
	getc(stdin);


    return(0);
}

static void bufferFunc(socketServer *server, const socketInfo *client){
	size_t i;
	for(i = 0; i < client->bufferArraySize; ++i){
		printf("Client #%u has sent: %s\n", client->id, client->bufferArray[i]);
	}
}

static void disconnectFunc(socketServer *server, const socketInfo *client){
	printf("Client #%u has been disconnected.\n", client->id);
}


static unsigned char loadServer(socketServer *server, const int type, const int protocol,
                                const char *configPath, void (*configFunc)(const char *configPath, char **ip, size_t *ipLength, unsigned short *port, size_t *bufferSize),
                                void (*discFunc)(socketServer *server, const socketInfo *client)){

	char *ip = NULL;
	size_t ipLength = 0;
	unsigned short port = DEFAULT_PORT;
	size_t bufferSize = DEFAULT_BUFFER_SIZE;
	(*configFunc)(configPath, &ip, &ipLength, &port, &bufferSize);

	const unsigned char success = serverInit(server, type, protocol, ip, ipLength, port, bufferSize, discFunc);

	free(ip);
	return(success);
}

static void loadConfig(const char *configPath, char **ip, size_t *ipLength, unsigned short *port, size_t *bufferSize){
	FILE *configFile = fopen(configPath, "rb");
	if(configFile != NULL){
		char lineBuffer[1024];
		char *line;
		size_t lineLength;

		unsigned int tempNum;
		char *endPos;


		while((line = readLineFile(configFile, lineBuffer, &lineLength)) != NULL){
			//Set the I.P.
			if(strncmp(line, "ip = ", 5) == 0){
				*ipLength = lineLength - 5;
				*ip = realloc(*ip, (*ipLength + 1) * sizeof(**ip));
				if(*ip != NULL){
					memcpy(*ip, line + 5, *ipLength * sizeof(**ip));
					(*ip)[*ipLength] = '\0';
				}

			//Set the port.
			}else if(strncmp(line, "port = ", 7) == 0){
				tempNum = strtol(line + 7, &endPos, 10);
				//Make sure it's valid!
				if(*endPos == '\0' && errno != ERANGE && tempNum < USHRT_MAX){
					*port = tempNum;
				}

			//Set the buffer size.
			}else if(strncmp(line, "buff = ", 7) == 0){
				tempNum = strtol(line + 7, &endPos, 10);
				//Make sure it's valid!
				if(*endPos == '\0' && errno != ERANGE && tempNum > 0){
					*bufferSize = tempNum;
				}
			}
		}
	}else{
		printf("Unable to open config file!\n"
		       "Path: %s\n\n", configPath);
	}
	fclose(configFile);
}

//Read a line from a file, removing any unwanted stuff!
static char *readLineFile(FILE *file, char *line, size_t *lineLength){
	line = fgets(line, 1024, file);
	if(line != NULL){
		*lineLength = strlen(line);

		//Remove comments.
		char *tempPos = strstr(line, "//");
		if(tempPos != NULL){
			*lineLength -= *lineLength - (tempPos - line);
		}

		//"Remove" whitespace characters from the beginning of the line!
		tempPos = &line[*lineLength];
		while(line < tempPos && isspace(*line)){
			++line;
		}
		*lineLength = tempPos - line;

		//"Remove" whitespace characters from the end of the line!
		while(*lineLength > 0 && isspace(line[*lineLength - 1])){
			--*lineLength;
		}

		line[*lineLength] = '\0';
	}


	return(line);
}