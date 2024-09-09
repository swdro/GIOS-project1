#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <getopt.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>

#define BUFSIZE 2048

#define USAGE                                                \
  "usage:\n"                                                 \
  "  transferclient [options]\n"                             \
  "options:\n"                                               \
  "  -o                  Output file (Default cs6200.txt)\n" \
  "  -p                  Port (Default: 38484)\n"            \
  "  -h                  Show this help message\n"           \
  "  -s                  Server (Default: localhost)\n"      \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"output", required_argument, NULL, 'o'},
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv)
{
    char *hostname = "localhost";
    int option_char = 0;
    unsigned short portno = 38484;
    char *filename = "cs6200.txt";

    setbuf(stdout, NULL);

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:o:hx", gLongOptions, NULL)) != -1) {
        switch (option_char) {
        case 's': // server
            hostname = optarg;
            break;
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        case 'o': // filename
            filename = optarg;
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        }
    }

    if (NULL == hostname) {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == filename) {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
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
    if ((getAddrInfoStatus = getaddrinfo(hostname, port, &addrInfoHints, &serverInfo)) != 0) {
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

    // open file for writing O_TRUNC to truncate the file before writing
    int fileFd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    int totalReceived = 0;
    int receivedBytes = 0;
    char buffer[BUFSIZE] = {0}; 
    receivedBytes = recv(socketFd, buffer, BUFSIZE - 1, 0);
    while (receivedBytes != 0) {
        if (receivedBytes == -1) {
            close(socketFd);
            close(fileFd);
            fprintf(stderr, "error when receiving bytes");
            exit(1);
        }
        // write to file
        int bytesWritten = 0;
        while (bytesWritten < receivedBytes) {
            if (bytesWritten == -1) {
                close(socketFd);
                close(fileFd);
                fprintf(stderr, "error writing to file");
                exit(1);
            }
            bytesWritten += write(fileFd, buffer + bytesWritten, strlen(buffer));
        }
        totalReceived += receivedBytes;
        memset(buffer, 0, BUFSIZE);
        receivedBytes = recv(socketFd, buffer, BUFSIZE - 1, 0);
    }

    close(fileFd);
    close(socketFd);
    freeaddrinfo(serverInfo); // free server addrinfo struct
}

