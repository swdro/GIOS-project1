#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <getopt.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>

#define BUFSIZE 512

#define USAGE                                                        \
    "usage:\n"                                                         \
    "  echoserver [options]\n"                                         \
    "options:\n"                                                       \
    "  -m                  Maximum pending connections (default: 5)\n" \
    "  -p                  Port (Default: 48384)\n"                    \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"port",          required_argument,      NULL,           'p'},
    {"maxnpending",   required_argument,      NULL,           'm'},
    {"help",          no_argument,            NULL,           'h'},
    {NULL,            0,                      NULL,             0}
};


int main(int argc, char **argv) {
    int portno = 48384; /* port to listen on */
    int option_char;
    int maxnpending = 5;
  
    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "p:m:hx", gLongOptions, NULL)) != -1) {
        switch (option_char) {
        case 'm': // server
            maxnpending = atoi(optarg);
            break; 
        case 'p': // listen-port
            portno = atoi(optarg);
            break;                                        
        case 'h': // help
            fprintf(stdout, "%s ", USAGE);
            exit(0);
            break;
        default:
            fprintf(stderr, "%s ", USAGE);
            exit(1);
        }
    }

    setbuf(stdout, NULL); // disable buffering

    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }
    if (maxnpending < 1) {
        fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__, maxnpending);
        exit(1);
    }



  /* Socket Code Here */

  // message no longer than 15 bytes. Allocate char buffer[16];
  // only output should be response from server

    int getAddrInfoStatus;
    struct addrinfo addrInfoHints;
    struct addrinfo *serverInfo;

    // convert portno data type to char array
    char port[6] = {0}; // port cannot be larger than 5 digits + null terminator
    sprintf(port, "%d", portno);
    memset(&addrInfoHints, 0, sizeof(addrInfoHints));
    addrInfoHints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    addrInfoHints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    addrInfoHints.ai_flags = AI_PASSIVE;     // assign localhost address to socket structures 
    // perform error checking to look for valid entries in the linked list, see client/server for real examples
    if ((getAddrInfoStatus = getaddrinfo(NULL, port, &addrInfoHints, &serverInfo)) != 0) {
        fprintf(stderr, "error when calling getaddrinfo(): %s\n", gai_strerror(getAddrInfoStatus));
        exit(1);
    }

    // create socket
    int socketFd;
    int option = 1;
    socketFd = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
    /*
    had an issue where the socket was entering the TIME_WAIT state. I had to set 
    an option for the socket (SO_REUSEADDR) so the address can be reused. Solution found
    here: https://stackoverflow.com/questions/5106674/error-address-already-in-use-while-binding-socket-with-address-but-the-port-num
    */
    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (socketFd == -1) {
        fprintf(stderr, "error when creating socket: %s\n", strerror(errno));
        exit(1);
    }
    //printf("socket successfully created: %d\n", socketFd);

    // bind socket
    int bindResult = bind(socketFd, serverInfo->ai_addr, serverInfo->ai_addrlen);
    if (bindResult == -1) {
        close(socketFd);
        fprintf(stderr, "error on socket bind: %s\n", strerror(errno));
        exit(1);
    }
    //printf("socket successfully bound, bindResult: %d\n", bindResult);

    // listen on socket and accept n connections
    int listenResult = listen(socketFd, maxnpending);
    if (listenResult == -1) {
        close(socketFd);
        fprintf(stderr, "error on listen: %s\n", strerror(errno));
        exit(1);
    }
    //printf("listen result: %d\n", listenResult);

    while(1) {
        struct sockaddr_storage clientAddr;
        socklen_t clientAddrSize = 0;
        int clientFd = accept(socketFd, (struct sockaddr *)&clientAddr, &clientAddrSize);
        if (clientFd == -1) {
            close(socketFd);
            fprintf(stderr, "error on accept: %s\n", strerror(errno));
            exit(1);
        }
        //printf("client fd: %d\n", clientFd);

        int bufferSize = 16;
        char buffer[16] = {0}; // port cannot be larger than 5 digits + null terminator
        int recvBytes = recv(clientFd, buffer, bufferSize, 0);
        if (recvBytes == -1) {
            close(socketFd);
            fprintf(stderr, "error when receiving bytes");
            exit(1);
        }
        //printf("received message: \n\n%s\n", buffer);
        //printf("sending message: %s\n", buffer);

        //send message back to client 
        int bytesSent = send(clientFd, buffer, bufferSize, 0);
        //printf("bytes sent: %d\n", bytesSent);
        if (bytesSent == -1) {
            close(socketFd);
            fprintf(stderr, "Error sending message: %s\n", strerror(errno));
            exit(1);
        }
    }


    // cleanup
    close(socketFd);
    freeaddrinfo(serverInfo); // free server addrinfo struct

    return 0;
}
