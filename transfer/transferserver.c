#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUFSIZE 2048

#define USAGE                                                \
    "usage:\n"                                               \
    "  transferserver [options]\n"                           \
    "options:\n"                                             \
    "  -p                  Port (Default: 38484)\n"          \
    "  -f                  Filename (Default: 6200.txt)\n"   \
    "  -h                  Show this help message\n"         \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"filename", required_argument, NULL, 'f'},
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {NULL, 0, NULL, 0}};

// Beej's guide chapter 7.4
int sendAll(int fd, char *buffer, int *bufferLength) {
    int totalBytesSent = 0; // all bytes sent
    int totalBytesLeft = *bufferLength;
    int bytesSent; // bytes sent on current iteration

    while (totalBytesSent < *bufferLength) {
        bytesSent = send(fd, buffer + totalBytesSent, totalBytesLeft, 0);
        if (bytesSent == -1) { break; }
        totalBytesSent += bytesSent;
        totalBytesLeft -= bytesSent;
    }

    *bufferLength = totalBytesLeft;
    return bytesSent == -1 ? -1 : 0;
}

int main(int argc, char **argv)
{
    int option_char;
    int portno = 38484;             /* port to listen on */
    char *filename = "6200.txt"; /* file to transfer */

    setbuf(stdout, NULL); // disable buffering

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "p:hf:x", gLongOptions, NULL)) != -1) {
        switch (option_char) {
        case 'f': // file to transfer
            filename = optarg;
            break;
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        }
    }


    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }
    
    if (NULL == filename) {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    /* Socket Code Here */
    int maxpending = 20;
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
    int listenResult = listen(socketFd, maxpending);
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

        // open file and get file descriptor, S_IRUSR and S_IWUSR specifies read and write permission for file owner
        int fileFd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

        // read file to buffer
        char buffer[BUFSIZE] = {0};
        while (read(fileFd, buffer, BUFSIZE - 1) != 0) {
            //send file to client 
            int bufferLength = strlen(buffer);
            int sentBytes = sendAll(clientFd, buffer, &bufferLength);
            if (sentBytes == -1) {
                close(clientFd);
                close(socketFd);
                close(fileFd);
                fprintf(stderr, "error when sending file");
                exit(1);
            }
            memset(buffer, 0, BUFSIZE);
        }
        close(fileFd);
        close(clientFd);
    }

    // cleanup
    close(socketFd);
    freeaddrinfo(serverInfo); // free server addrinfo struct

    return 0;

}
