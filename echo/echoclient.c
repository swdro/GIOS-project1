#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>

/* Be prepared accept a response of this length */
#define BUFSIZE 512

#define USAGE                                                                       \
    "usage:\n"                                                                      \
    "  echoclient [options]\n"                                                      \
    "options:\n"                                                                    \
    "  -m                  Message to send to server (Default: \"Hello Spring!!\")\n" \
    "  -s                  Server (Default: localhost)\n"                           \
    "  -p                  Port (Default: 48384)\n"                                  \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"message", required_argument, NULL, 'm'},
    {"port", required_argument, NULL, 'p'},
    {"server", required_argument, NULL, 's'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv)
{
    int option_char = 0;
    char *message = "Hello Fall!!";
    char *hostname = "localhost";
    unsigned short portno = 48384;

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:m:hx", gLongOptions, NULL)) != -1) {
        switch (option_char) {
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        case 's': // server
            hostname = optarg;
            break;
        case 'm': // message
            message = optarg;
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

    setbuf(stdout, NULL); // disable buffering

    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    if (NULL == message) {
        fprintf(stderr, "%s @ %d: invalid message\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == hostname) {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    /* Socket Code Here */
    int getAddrInfoStatus;
    struct addrinfo addrInfoHints;
    struct addrinfo *serverInfo;

    // convert portno data type to char array
    char port[6] = {0}; // port cannot be larger than 5 digits + null terminator
    sprintf(port, "%d", portno);
    //printf("port: %s\n", port);

    memset(&addrInfoHints, 0, sizeof(addrInfoHints));
    addrInfoHints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    addrInfoHints.ai_socktype = SOCK_STREAM; // TCP
    addrInfoHints.ai_flags = AI_PASSIVE;     // localhost address to socket structures 

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
    an option for the socket (SO_REUSEADDR) so the address can be reused. 
    */
    if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1) {
        perror("error on setsockopt");
        exit(1);
    }
    if (socketFd == -1) {
        fprintf(stderr, "error when creating socket: %s\n", strerror(errno));
        exit(1);
    }
    //printf("socket successfully created: %d\n", socketFd);

    //connect to server
    int connectStatus = connect(socketFd, serverInfo->ai_addr, serverInfo->ai_addrlen);
    if (connectStatus == -1) {
        fprintf(stderr, "error when connecting to server: %s\n", strerror(errno));
        exit(1);
    }

    // send message to server
    int msgLength = strlen(message);
    int bytesSent = send(socketFd, message, msgLength, 0);
    if (bytesSent == -1) {
        fprintf(stderr, "Error sending message: %s\n", strerror(errno));
        exit(1);
    }

    int bufferSize = 16;
    char buffer[16] = {0}; 
    // receive message from server
    int recvBytes = recv(socketFd, buffer, bufferSize, 0);
    if (recvBytes == -1) {
        close(socketFd);
        fprintf(stderr, "error when receiving bytes");
        exit(1);
    }
    //printf("received %d bytes\n", recvBytes);

    // account for null terminator by iterating over buffer until either buffer size or null terminator is reached 
    for (int i = 0; i < bufferSize; i++) {
        if (buffer[i] == '\0') {
            break;
        }
        printf("%c", buffer[i]);
    }
    //printf("\n");

    close(socketFd);
    freeaddrinfo(serverInfo); // free server addrinfo struct
}
