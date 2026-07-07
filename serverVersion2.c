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

struct clientsList cl;
struct fileDescriptorsAndIP {
    int socket_fd;
    int client_fd;
	char * client_ip;
	int* EOF_found;
};

void* receive_thread(void* fds){
	int socket_fd = ((struct fileDescriptorsAndIP *)fds)->socket_fd;
    int client_fd = ((struct fileDescriptorsAndIP *) fds)->client_fd;
	int* EOF_found = ((struct fileDescriptorsAndIP *)fds)->EOF_found;
    char msg[MAX_MSG_LEN + 1];
    char print_msg[MAX_PRINT_MSG_LEN + 1];
	char * client_ip;
	int client_ip_len = strlen(((struct fileDescriptorsAndIP *) fds)->client_ip);
	client_ip = (char *) malloc(client_ip_len);
	strncpy( client_ip , ((struct fileDescriptorsAndIP *) fds)->client_ip, client_ip_len );
    while (1){
		ssize_t read_bytes = recvMessage(client_fd, msg, MAX_MSG_LEN);
        if (read_bytes == 0  && !(*EOF_found)) {
			snprintf(print_msg, sizeof(print_msg), "-- User %s has left the conversation\n", client_ip);
			printMsg(stdout, print_msg);
			close(client_fd);
			free(client_ip);
			fprintf(stdout, "Client disconnected. Terminating\n");
			exit(EXIT_FAILURE);
		}
		else if (read_bytes == -1 && !(*EOF_found)) {
			perror("recvMessage failure");
			close(client_fd);
			close(socket_fd);
			exit(EXIT_FAILURE);
		} else if (read_bytes == -2 && !(*EOF_found)) {
			snprintf(print_msg, sizeof(print_msg), "error: the message of the client cannot fit in the 'msg' buffer\n");
			printMsg(stderr, print_msg);
			close(client_fd);
			close(socket_fd);
			exit(EXIT_FAILURE);
		}
		if (*EOF_found) return NULL;
		snprintf(print_msg, sizeof(print_msg), "\033[35mUser %s\033[0m> %s\n> ", client_ip, msg);
		printMsg(stdout, print_msg);
        strcpy(msg, "");
    }
}

int main() {
	struct sockaddr_in addr;
    struct fileDescriptorsAndIP fds; //We group socket_fd and client_fd so we can pass them to thread
	int socket_option;

	char reply[MAX_MSG_LEN + 1];   // data required to read the IP address of the connected client
	int EOF_found;

	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	char client_ip[INET_ADDRSTRLEN];

	char print_msg[MAX_PRINT_MSG_LEN + 1];

	pthread_t receiver;  //New thread for receiving messages

	fds.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fds.socket_fd == -1) {
		perror("socket failure");
		exit(EXIT_FAILURE);
	}

	// necessary option for quick reusage of the port
	socket_option = 1;
	if (setsockopt(fds.socket_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option)) == -1) {
		close(fds.socket_fd);
		perror("setsockopt failure");
		exit(EXIT_FAILURE);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(fds.socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		close(fds.socket_fd);
		perror("bind failure");
		exit(EXIT_FAILURE);
	}

	if (listen(fds.socket_fd, SOMAXCONN) == -1) {
		close(fds.socket_fd);
		perror("listen failure");
		exit(EXIT_FAILURE);
	}

	printf("Listening on port %d\n", PORT);

	fds.client_fd = accept(fds.socket_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	if (fds.client_fd == -1) {
		close(fds.socket_fd);
		perror("accept failure");
		exit(EXIT_FAILURE);
	}

	inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
	fds.client_ip = client_ip;
	snprintf(print_msg, sizeof(print_msg), "-- User %s has joined the conversation\n", client_ip);
	printMsg(stdout, print_msg);

	EOF_found = 0;
	fds.EOF_found = &EOF_found;
    if (pthread_create(&receiver, NULL, receive_thread, &fds) != 0){
        perror("pthread_create failure");
        exit(EXIT_FAILURE);
    }

	while (1) {
		printMsg(stdout, "Reply> ");

        do {
			if (fgets(reply, sizeof(reply), stdin) == NULL) {
				EOF_found = 1;
				shutdown(fds.client_fd, SHUT_RDWR);
				close(fds.client_fd);
				shutdown(fds.socket_fd, SHUT_RDWR);
				close(fds.socket_fd);
				fprintf(stdout, "\n");
				break;
			}
			reply[strcspn(reply, "\n")] = '\0';
		} while (strlen(reply) == 0 && EOF_found == 0);
		if (EOF_found) break;
        if (sendMessage(fds.client_fd, reply) == -1) {
			perror("sendMessage failure");
			close(fds.client_fd);
            close(fds.socket_fd);
            exit(EXIT_FAILURE);
        }
		strcpy(reply, "");
	}
    pthread_join(receiver, NULL);
	close(fds.socket_fd);
	return 0;
}