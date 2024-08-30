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

    printf("hello world\n");


  /* Socket Code Here */

  // message no longer than 15 bytes. Allocate char buffer[16];
  // only output should be response from server

    //int getAddrInfoStatus;
    //struct addrinfo addrInfoHints;
    //struct addrinfo *serverInfo;

    //// convert portno data type to char array
    //char port[6] = {0, 0, 0, 0, 0, 0};
    //sprintf(port, "%d", portno);
    //printf("port: %s", port);

    //memset(&addrInfoHints, 0, sizeof(addrInfoHints));
    //addrInfoHints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    //addrInfoHints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    //addrInfoHints.ai_flags = AI_PASSIVE;     // assign localhost address to socket structures 

    //if ((getAddrInfoStatus = getaddrinfo(NULL, port, &addrInfoHints, &serverInfo)) != 0) {
        //fprintf(stderr, "error when calling getaddrinfo(): %s\n", gai_strerror(getAddrInfoStatus));
        //exit(1);
    //}

    //freeaddrinfo(serverInfo); // free server addrinfo struct

}
