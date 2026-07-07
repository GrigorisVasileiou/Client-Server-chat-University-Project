#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include <stdio.h>

#include "readwrite.h"
#include "config.h"

ssize_t sendMessage(int fd, const char *msg)
{
	size_t msg_len = strlen(msg);
	uint32_t net_msg_len = htonl(msg_len);

	if (writeall(fd, &net_msg_len, sizeof(net_msg_len)) == -1) {
		return -1;
	}

	return writeall(fd, msg, msg_len);
}

ssize_t recvMessage(int fd, char *msg, size_t max_len)
{
	ssize_t nread;
	uint32_t net_msg_len, msg_len;

	nread = readall(fd, &net_msg_len, sizeof(net_msg_len));
	if (nread == 0 || nread == -1) {
		return nread;
	}
	msg_len = ntohl(net_msg_len);

	if (msg_len >= max_len) {  
		return -2;
	}

	if (msg_len > MAX_MSG_LEN) {
		return -3;
	}

	nread = readall(fd, msg, msg_len);
	if (nread == 0 || nread == -1) {
		return nread;
	}

	msg[nread] = '\0';

	return nread;
}

ssize_t sendNewMessage(int fd, const char *msg, const char *sender)
{
	size_t sender_len = strlen(sender);
	uint32_t net_sender_len = htonl(sender_len);
	
	size_t msg_len = strlen(msg);
	uint32_t net_msg_len = htonl(msg_len);

	size_t forSender = 0;
	size_t forMessage = 0;
	
	if (writeall(fd, &net_sender_len, sizeof(net_sender_len)) == -1){
		perror("Sending sender's name has failed");
		return -1;
	}
	forSender = writeall(fd, sender, sender_len);
	writeall(0, NULL, 0);
	if (writeall(fd, &net_msg_len, sizeof(net_msg_len)) == -1) {
		perror("Sending message has failed");
		return -1;
	}
	forMessage = writeall(fd, msg, msg_len);
	writeall(0, NULL, 0);
	return (forSender+forMessage);
}

ssize_t recvNewMessage(int fd, char *msg, size_t msg_max_len, char *sender, size_t sender_max_len)
{
	uint32_t net_sender_len;
	size_t sender_len;
	ssize_t name_read;

	name_read = readall(fd, &net_sender_len, sizeof(net_sender_len));
	if (name_read <= 0) {
		return name_read;
	}
	sender_len = ntohl(net_sender_len);

	if (sender_len >= sender_max_len) {
		return -30;
	}

	if (sender_len > MAX_CLIENT_NAME_LEN){
		return -7;
	}

	name_read = readall(fd, sender, sender_len);
	if (name_read <= 0) return name_read;

	uint32_t net_msg_len;
	size_t msg_len;
	ssize_t nread;
	nread = readall(fd, &net_msg_len, sizeof(net_msg_len));
	if (nread <= 0) {
		return nread;
	}

	msg_len = ntohl(net_msg_len);

	if (msg_len >= msg_max_len) {
		return -4;
	}

	if (msg_len > MAX_MSG_LEN) {
		return -3;
	}

	nread = readall(fd, msg, msg_len);
	if (nread <= 0) {
		return nread;
	}

	msg[nread+name_read] = '\0';
	return (name_read + nread);
}
