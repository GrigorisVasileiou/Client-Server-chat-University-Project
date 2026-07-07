CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2
LDFLAGS = -pthread

all: client server clientVersion1 serverVersion1 clientVersion2 serverVersion2 clientVersion3 serverVersion3

client: client.c msg.c config.h readwrite.h msg.h printMsg.h
	$(CC) $(CFLAGS) -o $@ client.c readwrite.c msg.c clientsList.c printMsg.c $(LDFLAGS)

server: server.c msg.c clientsList.c config.h readwrite.h msg.h clientsList.h printMsg.h
	$(CC) $(CFLAGS) -o $@ server.c readwrite.c msg.c clientsList.c printMsg.c $(LDFLAGS)

clientVersion1: clientVersion1.c msg.c config.h readwrite.h msg.h printMsg.h
	$(CC) $(CFLAGS) -o $@ clientVersion1.c readwrite.c msg.c clientsList.c printMsg.c $(LDFLAGS)

serverVersion1: serverVersion1.c msg.c clientsList.c config.h readwrite.h msg.h clientsList.h printMsg.h
	$(CC) $(CFLAGS) -o $@ serverVersion1.c readwrite.c msg.c clientsList.c printMsg.c $(LDFLAGS)

clientVersion2: clientVersion2.c msg.c config.h readwrite.h msg.h printMsg.h
	$(CC) $(CFLAGS) -o $@ clientVersion2.c readwrite.c msg.c clientsList.c printMsg.c $(LDFLAGS)

serverVersion2: serverVersion2.c msg.c clientsList.c config.h readwrite.h msg.h clientsList.h printMsg.h
	$(CC) $(CFLAGS) -o $@ serverVersion2.c readwrite.c msg.c clientsList.c printMsg.c $(LDFLAGS)

clientVersion3: clientVersion3.c msg.c config.h readwrite.h msg.h printMsg.h
	$(CC) $(CFLAGS) -o $@ clientVersion3.c readwrite.c msg.c clientsList.c printMsg.c $(LDFLAGS)

serverVersion3: serverVersion3.c msg.c clientsList.c config.h readwrite.h msg.h clientsList.h printMsg.h infoForReceivers.h
	$(CC) $(CFLAGS) -o $@ serverVersion3.c readwrite.c msg.c clientsList.c printMsg.c infoForReceivers.c $(LDFLAGS)

clean:
	rm -f client server clientVersion1 serverVersion1 clientVersion2 serverVersion2 serverVersion3 clientVersion3

.PHONY: all clean
