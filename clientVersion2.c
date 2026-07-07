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
#include "clientsList.h"
#include "printMsg.h"

struct infoPassed {
	int socket_fd;
	int* EOF_found;
};

void* receive_thread(void* args){
    int* EOF_found = ((struct infoPassed *)args)->EOF_found;
	int socket_fd = ((struct infoPassed *)args)->socket_fd;
    while (!(*EOF_found)){
        char msg[MAX_MSG_LEN + 1];
        char print_msg[MAX_PRINT_MSG_LEN + 1];
        ssize_t read_bytes = recvMessage(socket_fd, msg, MAX_MSG_LEN);
        if (read_bytes <= 0){
			if (!(*EOF_found)){
				fprintf(stdout, "Server disconnected.\n");
				exit(EXIT_FAILURE);
			}
			break;
        }
        snprintf(print_msg, sizeof(print_msg), "Server> %s\n> ", msg);
        printMsg(stdout, print_msg);
        strcpy(msg, "");
    }
    return NULL;
}

int main()
{
	struct sockaddr_in addr;
	int socket_fd;
	char msg[MAX_MSG_LEN + 1];
	char print_msg[MAX_PRINT_MSG_LEN + 1];
	int EOF_found;

	struct infoPassed infoPassed;

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1) {
		perror("socket failure");
		exit(EXIT_FAILURE);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);

	if (inet_pton(AF_INET, SERVER_IP_ADDRESS, &addr.sin_addr) <= 0) {
	    close(socket_fd);
	    perror("inet_pton failure");
	    exit(EXIT_FAILURE);
	}

	if (connect(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		close(socket_fd);
	    perror("connect failure");
	    exit(EXIT_FAILURE);
	}

	EOF_found = 0;
    pthread_t receiver;  //New thread for receiving messages
	infoPassed.socket_fd = socket_fd;
	infoPassed.EOF_found = &EOF_found;
    if (pthread_create(&receiver, NULL, receive_thread, &infoPassed) != 0){
        fprintf(stdout ,"pthread_create failure");
        exit(EXIT_FAILURE);
    }
    
	do {
		do {
			snprintf(print_msg, sizeof(print_msg), "> ");
			printMsg(stdout, print_msg);
			
			if(fgets(msg, sizeof(msg), stdin) == NULL) {
				EOF_found = 1;
				fprintf(stdout, "Terminating connection... Exiting\n");
				shutdown(socket_fd, SHUT_RDWR);
				pthread_join(receiver, NULL); //Wait for receiver thread to terminate
				close(socket_fd);
				break;
			}
		
			msg[strcspn(msg, "\n")] = '\0';
		} while (!EOF_found && strlen(msg) == 0);
        
		if (!EOF_found) {
			if (sendMessage(socket_fd, msg) == -1) {
				close(socket_fd);
				fprintf(stdout, "sendMessage failure");
				exit(EXIT_FAILURE);
			}
		}
	} while (!EOF_found);
	return 0;
}