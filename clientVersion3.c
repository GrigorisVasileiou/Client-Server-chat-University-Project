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

struct infoForReceiver{
	int* socket_fd;
	int* EOF_found;
	char* msg;
};

void* receive_thread(void* infoForReceiver){
	int* socket_fd = ((struct infoForReceiver*)infoForReceiver)->socket_fd;
	int* EOF_found = ((struct infoForReceiver*)infoForReceiver)->EOF_found;
    while (!(*EOF_found)){
        char msg[MAX_MSG_LEN + 1];
        char print_msg[MAX_PRINT_MSG_LEN + 1];
		char name[MAX_CLIENT_NAME_LEN + 1];
		memset(name, 0, MAX_CLIENT_NAME_LEN+1);
		memset(msg, 0, MAX_MSG_LEN+1);
		memset(print_msg, 0, MAX_PRINT_MSG_LEN + 1);
        ssize_t read_bytes = recvNewMessage(*socket_fd, msg, MAX_MSG_LEN, name, MAX_CLIENT_NAME_LEN);
        if (read_bytes <= 0){
			if (*EOF_found == 1)
				fprintf(stdout ,"I have disconnected\n");
			else{
				fprintf(stdout, "Server disconnected! Terminating\n");
				close(*socket_fd);
				exit(EXIT_FAILURE);
			}
			break;
        }
        if (!(*EOF_found))
        	snprintf(print_msg, sizeof(print_msg), "\033[35m%s\033[0m> %s\n> ", name, msg);
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
	struct infoForReceiver infoForReceiver;

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

	infoForReceiver.socket_fd = &socket_fd;
	infoForReceiver.EOF_found = &EOF_found;
	infoForReceiver.msg = msg;
    if (pthread_create(&receiver, NULL, receive_thread, &infoForReceiver  ) != 0){
        fprintf(stderr, "Failed at creating thread for receiving messages. Terminating client.");
        exit(EXIT_FAILURE);
    }
    
	do {
		do {
				snprintf(print_msg, sizeof(print_msg), "> ");
				printMsg(stdout, print_msg);

			if (fgets(msg, sizeof(msg), stdin) == NULL) {
				EOF_found = 1;
				shutdown(socket_fd, SHUT_RDWR); //make receiver thread move on
				break;
			}
			msg[strcspn(msg, "\n")] = '\0';
		} while (strlen(msg) == 0 && !EOF_found);
        
		if (!EOF_found) {
			if (sendMessage(socket_fd, msg) == -1) {
					for (int i = 0; i < 3;){
						if (sendMessage(socket_fd, msg) == -1) 
							i++; 
						else break;
						if (i == 2){
							perror("sendMessage failure");
							exit(EXIT_FAILURE);
						}
					}
			}
		}
	} while (!EOF_found);
	if (pthread_join(receiver, NULL) != 0){
		fprintf(stderr ,"Failed at joining receiver thread with main thread. Terminating client.");
		close(socket_fd);
		exit(EXIT_FAILURE);
	}
	close(socket_fd);
	return 0;
}
