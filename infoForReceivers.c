#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "infoForReceivers.h"

pthread_mutex_t lockForAllClientsInfo;

int initializeAllClientsInfo(struct clientInfoPassed ** allClientsInfo[]){
	pthread_mutex_init(&lockForAllClientsInfo, NULL);
	for (int i = 0; i < MAX_CONNECTED_CLIENTS; i++){
		(*allClientsInfo)[i] = NULL;
	}
	return 0;
}

int addReceiverThreadInfo(struct clientInfoPassed** allClientsInfo[], struct clientInfoPassed** clientInfoForAddition){
	pthread_mutex_lock(&lockForAllClientsInfo);
	for (int i = 0; i < MAX_CONNECTED_CLIENTS; i++){
		if ((*allClientsInfo)[i] == NULL){
			(*allClientsInfo)[i] = *clientInfoForAddition;
			pthread_mutex_unlock(&lockForAllClientsInfo);
			return 0;
		}
	}
	pthread_mutex_unlock(&lockForAllClientsInfo);
	fprintf(stderr, "Failed to add new client info in allClientsInfo");
	return -1;
}

int removeReceiverThreadInfo(struct clientInfoPassed ** allClientsInfo[], struct clientInfoPassed** clientInfoForRemoval){
	pthread_mutex_lock(&lockForAllClientsInfo);
	for (int i = 0; i < MAX_CONNECTED_CLIENTS; i++){
		if ((*allClientsInfo)[i] == (*clientInfoForRemoval)){
			close(*(*clientInfoForRemoval)->client_fd);
			removeClient((*clientInfoForRemoval)->allClients, *((*clientInfoForRemoval)->client_fd));
			free((*clientInfoForRemoval)->client_fd);
			free((*clientInfoForRemoval)->threadAddress);
			free(*clientInfoForRemoval);
			(*allClientsInfo)[i] = NULL;
			pthread_mutex_unlock(&lockForAllClientsInfo);
			return 0;
		}
	}
	pthread_mutex_unlock(&lockForAllClientsInfo);
	fprintf(stderr, "Failed to remove info for a client from allClientsInfo\n");
	return -1;
}

int numberOfRemainingClients(struct clientInfoPassed ** allClientsInfo[]){
	int j = 0;
	pthread_mutex_lock(&lockForAllClientsInfo);
	for (int i = 0; i < MAX_CONNECTED_CLIENTS; i++){
		if ((*allClientsInfo)[i] != NULL) j++;
	}
	pthread_mutex_unlock(&lockForAllClientsInfo);
	return j;
}

int destroyAllClientsInfo(struct clientInfoPassed ** allClientsInfo[]){
	pthread_mutex_destroy(&lockForAllClientsInfo);
	free(*allClientsInfo);
	return 0;
}