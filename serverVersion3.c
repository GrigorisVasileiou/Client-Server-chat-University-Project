#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "readwrite.h"
#include "msg.h"
#include "printMsg.h"
#include "infoForReceivers.h"

void* receive_thread(void* clientInfo){
	struct clientInfoPassed * infoPassed = (struct clientInfoPassed*) clientInfo;
	int *client_fd = infoPassed->client_fd;
	int *socket_fd = infoPassed->socket_fd;
	struct clientsList* allClients = infoPassed->allClients;
	struct clientInfoPassed*** structureWithAllInfoPassed = infoPassed->structureWithAllInfoPassed;
    char msg[MAX_MSG_LEN + 1];
    char* client_name;
    client_name = (char *) calloc((MAX_CLIENT_NAME_LEN+1), sizeof(char));
	if (client_name == NULL){
		fprintf(stderr, "Allocating memory for client with file descriptor %ls failed. Terminating connection\n", client_fd);
		removeReceiverThreadInfo(structureWithAllInfoPassed, &infoPassed);
    	return NULL;
	}
    char client_filedescriptorForDeafultName[64];
    sprintf(client_filedescriptorForDeafultName, "%d", ((*client_fd) - 3)); //Make a client have a default name
    strcat(client_name, client_filedescriptorForDeafultName);
    fprintf(stdout, "%s has connected\n", client_name);
    while (1){
    	memset(msg, 0, MAX_MSG_LEN+1);
        ssize_t read_bytesNew = recvMessage(*client_fd, msg, MAX_MSG_LEN); //The last variable might not be MAX_MSG_LEN and might have to be sent by the sender
		char print_msg[MAX_PRINT_MSG_LEN + 1];
        if (strncmp("\\name ", msg, 6) == 0){
			int i;
			memset(client_name, 0, MAX_CLIENT_NAME_LEN+1); //Clear client's name before change
			for (i = 0; i < (MAX_CLIENT_NAME_LEN+1) && (msg[i+6] != '\0'); i++){
				client_name[i] = msg[i+6];
			}
			client_name[i] = '\0';
            continue;
        }
		if (read_bytesNew > 0) {
    		broadcastClientsList(allClients, msg, client_name, *client_fd); //We want to broadcast the message that has been received to everyone else
		}
        if (read_bytesNew == 0) {
			snprintf(print_msg, sizeof(print_msg), "-- User %s has left the conversation\n", client_name);
			printMsg(stdout, print_msg);
			char stringForOthers[35+MAX_CLIENT_NAME_LEN];
			strncpy(stringForOthers ,"--User ", 8);
			strcat(stringForOthers, client_name);
			strcat(stringForOthers, " has left the conversation");
			broadcastClientsList(allClients, stringForOthers, client_name, *client_fd);
			pthread_mutex_lock(&(allClients->clients_lock));
			int numOfClients = allClients->clients_num;
			fprintf(stdout , "Number of clients is %d\n", (numOfClients-1));
			if (numOfClients == 1){
				pthread_mutex_unlock(&(allClients->clients_lock));
				shutdown(*socket_fd, SHUT_RDWR); //Make main thread move on
				printf("No more clients to serve. Terminating... \n");
			}else{
				pthread_mutex_unlock(&(allClients->clients_lock));
				removeReceiverThreadInfo(structureWithAllInfoPassed, &infoPassed);
			}
			free(client_name);
			return NULL;
		}
		else if (read_bytesNew == -1) {
			perror("recvMessage failure");
			close(*client_fd);
			free(client_name);
			removeReceiverThreadInfo(structureWithAllInfoPassed, &infoPassed);
			return NULL;
		} else if (read_bytesNew == -2) {
			snprintf(print_msg, sizeof(print_msg), "error: the message of the client cannot fit in the 'msg' buffer\n");
			printMsg(stderr, print_msg);
			close(*client_fd);
			free(client_name);
			removeReceiverThreadInfo(structureWithAllInfoPassed, &infoPassed);
			return NULL;
		}
		snprintf(print_msg, sizeof(print_msg), "\033[35mUser %s\033[0m> %s\n> ", client_name, msg);
		printMsg(stdout, print_msg);
        strcpy(msg, "");
    }
}

int main() {
	struct sockaddr_in addr;
	int socket_fd;
	int socket_option;
	
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	struct clientInfoPassed** AllReceiverThreadsInfo;

	AllReceiverThreadsInfo = (struct clientInfoPassed **) malloc(MAX_CONNECTED_CLIENTS * sizeof(struct clientInfoPassed *)); //Allocate memory to store a structure
	//in which every information necessary for reciver threads is stored
	if (AllReceiverThreadsInfo == NULL){
		fprintf(stderr, "Allocating memory for structure to store all information for receiver threads failed. Terminating\n");
		return 0;
	}

	initializeAllClientsInfo(&AllReceiverThreadsInfo);
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1) {
		perror("socket failure");
		exit(EXIT_FAILURE);
	}

	// Necessary option for quick reusage of the port
	socket_option = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option)) == -1) {
		close(socket_fd);
		perror("setsockopt failure");
		exit(EXIT_FAILURE);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		close(socket_fd);
		perror("bind failure");
		exit(EXIT_FAILURE);
	}

	if (listen(socket_fd, SOMAXCONN) == -1) {
		close(socket_fd);
		perror("listen failure");
		exit(EXIT_FAILURE);
	}
	printf("Listening on port %d.\n", PORT);

    struct clientsList clients; //List of clients
    initClientsList(&clients); //Intitializing list of clients
	while (1) {
        int *client_fd;
		client_fd = (int *) malloc(sizeof(int));
		if (client_fd == NULL){
			fprintf(stderr, "Allocating memory to store a client's file descriptor failed. Retrying\n");
			continue;
		}

		*client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);
		if (*client_fd == -1){
			if (numberOfRemainingClients(&AllReceiverThreadsInfo) == 1) break;
			fprintf(stderr, "Accepting client failed\n");
			continue;
		}
        if (addClient(&clients, *client_fd) == -1){
            perror("accept failure. no available space for new clients. try later");
			free(client_fd);
			break;
        }

		pthread_mutex_lock(&clients.clients_lock);
		fprintf(stdout ,"New client connected number of clients is %d\n", clients.clients_num);
		pthread_mutex_unlock(&clients.clients_lock);

        struct clientInfoPassed * infoAboutClients;
        infoAboutClients = (struct clientInfoPassed *) malloc(sizeof(struct clientInfoPassed));
		if (infoAboutClients == NULL){
			fprintf(stderr, "Allocating memory to store info for client failed\n");
			close(*client_fd);
			free(client_fd);
			continue;
		}
		infoAboutClients->client_fd = (int *) malloc(sizeof(int));
		if (infoAboutClients->client_fd == NULL){
			fprintf(stderr, "Allocating memory to store info for client's file descriptor failed\n");
			close(*client_fd);
			free(client_fd);
			free(infoAboutClients);
			continue;
		}
		memcpy(infoAboutClients->client_fd, client_fd, sizeof(int));
        infoAboutClients->allClients = &clients;
		infoAboutClients->socket_fd = &socket_fd;
		infoAboutClients->threadAddress = (pthread_t *) malloc(sizeof(pthread_t));
		if (infoAboutClients->threadAddress == NULL){
			fprintf(stderr, "Allocating memory to store info for client failed\n");
			close(*client_fd);
			free(client_fd);
			free(infoAboutClients->client_fd);
			free(infoAboutClients);
			continue;
		}
		infoAboutClients->structureWithAllInfoPassed = &AllReceiverThreadsInfo;
		addReceiverThreadInfo(&AllReceiverThreadsInfo, &infoAboutClients);
		if (pthread_create((infoAboutClients->threadAddress), NULL, receive_thread, infoAboutClients) != 0){
			fprintf(stderr ,"Creating thread %p to handle client with file descriptor %ls failed\n", infoAboutClients->threadAddress, client_fd);
			removeReceiverThreadInfo(&AllReceiverThreadsInfo, &infoAboutClients);
			continue;
		}
		if (pthread_detach(*(infoAboutClients->threadAddress)) != 0){
			fprintf(stderr, "Detatching thread %p which handles client with file descriptor %ls failed\n", infoAboutClients->threadAddress, client_fd);
			close(*(infoAboutClients->client_fd));
			removeReceiverThreadInfo(&AllReceiverThreadsInfo, &infoAboutClients);
			continue;
		}
		free(client_fd);
	}

	pthread_t * lastReceiverThread = NULL;
	struct clientInfoPassed* lastRemainingClientInfo;

	for (int i = 0; i<MAX_CONNECTED_CLIENTS ;i++){
		if (AllReceiverThreadsInfo[i] == NULL){
			continue;
		}
		lastReceiverThread = AllReceiverThreadsInfo[i]->threadAddress;
		lastRemainingClientInfo = AllReceiverThreadsInfo[i];
	}

	pthread_join(*lastReceiverThread, NULL);
	removeReceiverThreadInfo(&AllReceiverThreadsInfo, &lastRemainingClientInfo);
	destroyAllClientsInfo(&AllReceiverThreadsInfo); //Destroying list of clients
	close(socket_fd); //Closing socket file descriptor
	return 0;
}
