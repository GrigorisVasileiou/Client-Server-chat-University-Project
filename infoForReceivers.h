#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "clientsList.h"

#ifndef GLOBAL_VAR
#define GLOBAL_VAR

 extern pthread_mutex_t lockForAllClientsInfo;

#endif

struct clientInfoPassed {
    struct clientsList * allClients;
    int *client_fd;
	int *socket_fd;
	pthread_t* threadAddress;
	struct clientInfoPassed*** structureWithAllInfoPassed;
};

int initializeAllClientsInfo(struct clientInfoPassed ** allClientsInfo[]);
int addReceiverThreadInfo(struct clientInfoPassed** allClientsInfo[], struct clientInfoPassed** clientInfoForAddition);
int removeReceiverThreadInfo(struct clientInfoPassed ** allClientsInfo[], struct clientInfoPassed** clientInfoForRemoval);
int destroyAllClientsInfo(struct clientInfoPassed ** allClientsInfo[]);
int numberOfRemainingClients(struct clientInfoPassed ** allClientsInfo[]);