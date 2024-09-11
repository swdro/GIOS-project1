#include "gfserver-student.h"
#include <stdlib.h>
#include <fcntl.h>

// Modify this file to implement the interface specified in
 // gfserver.h.

struct gfserver_t {
    char *port;
    int maxnpending;
    void *handlerarg; 
    gfh_error_t (*handler)(gfcontext_t **, const char *, void*);
};

struct gfcontext_t {
    int clientFd;
};

void gfs_abort(gfcontext_t **ctx){
}

ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len){
    // not yet implemented
    fprintf(stdout, "handler called: gfs_send(<ctx>, <data>, %lu)\n", len);
    char buffer[len];
    memcpy(&buffer, data, len);
    buffer[len-1] = '\0';
    fprintf(stdout, "handle passed <data>: '%s'\n", buffer);
    return 1034;
}

ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len){
    // not yet implemented
    fprintf(stdout, "handler called: gfs_sendheader(%d, %lu)\n", status, file_len);
    char *scheme = "GETFILE";
    char *endHeaderMarker = "\r\n\r\n";
    char fileLenStr[100];
    char *statusStr;
    switch (status) {
        case 200:
            statusStr = "OK";
        case 400: 
            statusStr = "FILE_NOT_FOUND";
        case 500:
            statusStr = "ERROR";
        case 600:
            statusStr = "INVALID";
    }
    itoa(file_len, fileLenStr, 10);

    // create header
    char header[100] = {0};
    strcat(header, scheme);
    strcat(header, ' ');
    strcat(header, statusStr);
    strcat(header, ' ');
    strcat(header, fileLenStr);
    strcat(header, endHeaderMarker);
    printf("header: %s\n", header);

    return strlen(header) + 1;
}

/*
 * This function must be the first one called as part of
 * setting up a server.  It returns a gfserver_t handle which should be
 * passed into all subsequent library calls of the form gfserver_*.  It
 * is not needed for the gfs_* call which are intended to be called from
 * the handler callback.
 */
gfserver_t *gfserver_create() {
    // not yet implemented
    gfserver_t* gfserver = malloc(sizeof(gfserver_t));
    memset(gfserver, 0, sizeof(gfserver_t));
    return gfserver;
    //return (gfserver_t *)NULL;
}

void gfserver_set_port(gfserver_t **gfs, unsigned short port){
    gfserver_t *gfserver = *gfs;
    char portStr[6] = {0}; // port cannot be larger than 5 digits + null terminator
    sprintf(portStr, "%d", port);
    gfserver->port = portStr;
}

void gfserver_set_maxpending(gfserver_t **gfs, int max_npending){
    gfserver_t *gfserver = *gfs;
    gfserver->maxnpending = max_npending;
}

void gfserver_set_handlerarg(gfserver_t **gfs, void* arg){
    gfserver_t *gfserver = *gfs;
    gfserver->handlerarg = arg;
}

void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void*)){
    gfserver_t *gfserver = *gfs;
    gfserver->handler = handler;
}

void gfserver_serve(gfserver_t **gfs){
    // server logic
    gfserver_t *gfserver = *gfs;
    int getAddrInfoStatus;
    struct addrinfo addrInfoHints;
    struct addrinfo *serverInfo;

    // convert portno data type to char array
    memset(&addrInfoHints, 0, sizeof(addrInfoHints));
    addrInfoHints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    addrInfoHints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    addrInfoHints.ai_flags = AI_PASSIVE;     // assign localhost address to socket structures 
    // perform error checking to look for valid entries in the linked list, see client/server for real examples
    if ((getAddrInfoStatus = getaddrinfo(NULL, gfserver->port, &addrInfoHints, &serverInfo)) != 0) {
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
    int listenResult = listen(socketFd, gfserver->maxnpending);
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

        gfcontext_t *gfcontext = malloc(sizeof(gfcontext_t));
        gfcontext->clientFd = clientFd;
        printf("gf context : %d\n", gfcontext->clientFd);

        gfserver->handler(&gfcontext, "/courses/ud923/filecorpus/1kb-sample-file-1.html", gfserver->handlerarg);
        printf("mesage sent successfully");

        /*
        - receive the request
        - handle the request and send necesary information to the handler (gfcontext_t **, const char *, void*)
        - close client socket connection, cleanup
        */

        // ***** below code should be implemented in the handler

        // open file and get file descriptor, S_IRUSR and S_IWUSR specifies read and write permission for file owner
        /*
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
        */

        // ***** above code should be implemented in the handler


    }
}

