# Client-Server-chat-University-Project
University project implementing a multithreaded client-server chat application in C.

# Client-Server Chat University Project

This project was developed as part of a Computer Networks university course.

It implements a TCP client-server chat application in C using POSIX sockets and pthreads. The project was completed in multiple stages, gradually extending the functionality from a simple request-response
protocol to a multithreaded chat server supporting multiple clients.

## Features

- TCP client-server communication
- Request-response communication
- Concurrent message handling using POSIX threads
- Multithreaded server (one thread per client)
- Support for multiple connected clients
- Message broadcasting
- Username change using the `\name` command
- Proper socket and memory management
- Graceful client disconnection

## Technologies

- C
- POSIX Sockets
- POSIX Threads (pthreads)
- Linux (Debian 12)

## Project Structure

```
client.c          Client implementation
server.c          Server implementation
clientsList.c     Connected clients management
msg.c             Message serialization
readwrite.c       Reliable socket I/O
printMsg.c        Thread-safe printing
Makefile          Project build
report.pdf        Complete project report
```

## Build

```bash
make
```

## Run

Server

```bash
./server
```

Client

```bash
./client
```

## Documentation

The complete implementation details, design decisions, code explanations and execution screenshots are available in **report.pdf**.

## Author

University project developed for the Computer Networks course.
