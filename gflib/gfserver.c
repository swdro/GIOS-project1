#include "gfserver-student.h"
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

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
    free(*ctx);
}

ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t len){
    // not yet implemented
    fprintf(stdout, "handler called: gfs_send(<ctx>, <data>, %lu)\n", len);
    int clientFd = (*ctx)->clientFd;
    printf("length: %zu\n", len);
    char buffer[len];
    memcpy(buffer, data, len);
    int bytesSent = sendAll(clientFd, buffer, len);
    if (bytesSent == -1) {
        printf("Response data failed to send: %s\n", strerror(errno));
        //abort
    }
    printf("successfully sent %d bytes\n", bytesSent);
    return bytesSent;
}

ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len){
    // not yet implemented
    fprintf(stdout, "handler called: gfs_sendheader(%d, %lu)\n", status, file_len);
    int clientFd = (*ctx)->clientFd;
    char *scheme = "GETFILE";
    char *endHeaderMarker = "\r\n\r\n";
    char fileLenStr[100];
    char *statusStr;
    switch (status) {
        case 200:
            statusStr = "OK";
            break;
        case 400: 
            statusStr = "FILE_NOT_FOUND";
            break;
        case 500:
            statusStr = "ERROR";
            break;
        case 600:
            statusStr = "INVALID";
            break;
    }
    sprintf(fileLenStr, "%d", (int) file_len);

    // create header string
    char header[100] = {0};
    strcat(header, scheme);
    strcat(header, " ");
    strcat(header, statusStr);
    strcat(header, " ");
    strcat(header, fileLenStr);
    strcat(header, endHeaderMarker);
    int headerLen = strlen(header);
    printf("header: %s\n", header);
    // send header and make sure all bytes have been sent
    int bytesSent = sendAll(clientFd, header, headerLen);
    if (bytesSent == -1) {
        printf("Response header failed to send: %s\n", strerror(errno));
    }
    printf("header length: %zu\nsuccessfully sent %d bytes\n", strlen(header), bytesSent);
    return bytesSent;
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

// define state functions
STATE_RETURN_CODE schemeState(char* currStr, char c) {
    //printf("SCHEME STATE char: %c\n", c);
    //printf("curr str: %s\n", currStr);
    int currLen = strlen(currStr);
    if (currLen == 7 && strcmp(currStr, "GETFILE") == 0) {
        return SUCCESS;
    }
    // we need to change currStr because 
    if (currLen >= 8) {
        return FAIL;
    }
    return REPEAT;
}

STATE_RETURN_CODE spaceState(char* currStr, char c) {
    printf("SPACE STATE char: %c\n", c);
    if (c == ' ') {
        return SUCCESS;
    }
    return FAIL;
}

STATE_RETURN_CODE methodState(char* currStr, char c) {
    int currLen = strlen(currStr);
    //printf("METHOD STATE char: %c\n", c);
    //printf("METHOD STATE currStr length: %d\n", currLen);
    //printf("curr str: %s\n", currStr);
    if (currLen == 3 && strcmp(currStr, "GET") == 0) {
        return SUCCESS;
    }
    else if (currLen == 1 && c != 'G') {
        return FAIL;
    }
    else if (currLen == 2 && c != 'E') {
        return FAIL;
    }
    else if (currLen > 3) {
        return FAIL;
    }
    return REPEAT;
}

STATE_RETURN_CODE secondSpaceState(char* currStr, char c) {
    printf("SECOND SPACE STATE char: %c\n", c);
    if (c == ' ') {
        return SUCCESS;
    }
    return FAIL;
}

STATE_RETURN_CODE pathState(char* currStr, char c) {
    int currStrLen = strlen(currStr);
    //printf("currStr: %s\n", currStr);
    if (
        currStrLen >= 4 && 
        currStr[currStrLen - 1] == '\n' &&
        currStr[currStrLen - 2] == '\r' &&
        currStr[currStrLen - 3] == '\n' &&
        currStr[currStrLen - 4] == '\r'
    ) {
        printf("passed path state: %s\n", currStr);
        currStr[currStrLen - 1] = 0;
        currStr[currStrLen - 2] = 0;
        currStr[currStrLen - 3] = 0;
        currStr[currStrLen - 4] = 0;
        return SUCCESS;
    }
    return REPEAT;
}

// state functions
typedef STATE_RETURN_CODE (*GetFileStateFunc)(char *currStr,  char c);
GetFileStateFunc GetFileStateFunctions[] = {
    schemeState,
    spaceState,
    methodState,
    secondSpaceState,
    pathState,
};

// define different states
typedef enum {
    SCHEME,
    SPACE,
    METHOD,
    SECONDSPACE,
    PATH,
    END
} GETFILESTATE;

// transition struct definition 
typedef struct {
    GETFILESTATE sourceState;
    STATE_RETURN_CODE returnCode;
    GETFILESTATE destinationState;
} Transition;

// transition array to define transitions
Transition transitionStates[][3] = {
{
    {SCHEME, SUCCESS, SPACE}, 
    {SCHEME, REPEAT, SCHEME},
},
{
    {SPACE, SUCCESS, METHOD}, 
    {SPACE, REPEAT, SPACE},
},
{
    {METHOD, SUCCESS, SECONDSPACE}, 
    {METHOD, REPEAT, METHOD},
},
{
    {SECONDSPACE, SUCCESS, PATH}, 
    {SECONDSPACE, REPEAT, SECONDSPACE},
},
{
    {PATH, SUCCESS, END}, 
    {PATH, REPEAT, PATH},
},
};

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

        GETFILESTATE currState = SCHEME;
        size_t getFileMessageCapacity = 2048;
        size_t tempFileMessageCapacity = 2048;
        char *getFileMessage = calloc(getFileMessageCapacity, sizeof(char));
        char *tempFileMessage = calloc(getFileMessageCapacity, sizeof(char));
        int recvBytes = recv(clientFd, getFileMessage, getFileMessageCapacity-1, 0);
        int getFileMessageIndex = 0;
        int tempFileMessageIndex = 0;
        STATE_RETURN_CODE rc;

        while (recvBytes != 0) {
            if (recvBytes == -1) {
                close(socketFd);
                fprintf(stderr, "error when receiving bytes");
                exit(1);
            }
            printf("getFileMessage: %s\n", getFileMessage);
            // iterate and verify newly received bytes
            while (getFileMessageIndex < strlen(getFileMessage)) {
                //printf("getFileMessageIndex: %d\n", getFileMessageIndex);
                //printf("getFileMessageLength: %ld\n", strlen(getFileMessage));
                char currGetFileChar = getFileMessage[getFileMessageIndex]; // get current char
                //printf("currentGetFileChar: %c\n", currGetFileChar);
                tempFileMessage[tempFileMessageIndex] = currGetFileChar; // add char to tempFileMessage
                GetFileStateFunc stateFunc = GetFileStateFunctions[currState]; // call state functions
                rc = stateFunc(tempFileMessage, currGetFileChar);
                //printf("Result: %d\n", rc);
                getFileMessageIndex++; // move to next character
                tempFileMessageIndex++;
                for (int i = 0; i < 2; i++) { // change 2 to whatever the size of inner most keys_array_ptr
                    Transition temp_state = transitionStates[currState][i];
                    if (temp_state.returnCode == rc) {
                        currState = temp_state.destinationState; // advance state
                        printf("new currState: %d\n", currState);
                        if (rc == SUCCESS && currState != END) {
                            printf("SUCCESS on state %d.. \n", currState);
                            memset(tempFileMessage, 0, tempFileMessageCapacity);
                            tempFileMessageIndex = 0;
                            //free(curr_str);
                            //curr_str = calloc(1, sizeof(char));
                            //curr_str[0] = '\0';
                        }
                        break;
                    }
                }
                if (rc == FAIL) {
                    printf("Failed on state %d, returning.. \n", currState);
                    break; // associated function printed to stderr already
                }
            }
            
            // stop receiving bytes on success or fail
            if (currState == END || rc == FAIL) {
                break;
            }

            // get more memory for message if needed
            int getFileMessageSpaceLeft = getFileMessageCapacity - strlen(getFileMessage) - 1; // account for null term
            if (getFileMessageSpaceLeft == 0) {
                getFileMessageSpaceLeft += getFileMessageCapacity; // since we double capacity, add current capactiy to current space left
                getFileMessageCapacity *= 2;
                getFileMessage = realloc(getFileMessage, sizeof(char) * getFileMessageCapacity);
            }
            int tempFileMessageSpaceLeft = tempFileMessageCapacity - strlen(tempFileMessage) - 1;
            if (tempFileMessageSpaceLeft == 0) {
                tempFileMessageSpaceLeft += tempFileMessageCapacity;
                tempFileMessageCapacity *= 2;
                tempFileMessage = realloc(tempFileMessage, sizeof(char) * tempFileMessageCapacity);
            }
            recvBytes = recv(socketFd, getFileMessage + strlen(getFileMessage), getFileMessageSpaceLeft, 0);
            getFileMessage[strlen(getFileMessage)] = 0; // set null terminator
        }

        printf("tempFileMessage to pass to handle: %s\n", tempFileMessage);
        gfserver->handler(&gfcontext, tempFileMessage, gfserver->handlerarg);
        printf("mesage sent successfully");
        free(gfcontext);
        free(tempFileMessage);
        free(getFileMessage);
        

        /*
        incomplete header:
        - we receive incomplete header..  handled by timeout
        - client closes connection, we return 0 and close in parent function call
        - recv fails, we return -1 and fail in parent

        returns: string that is not null terminated
        */
        //int BUFFSIZE = 300;
        //char buffer[300] = {0};
        ////memset(buffer, 0, 300);
        //int receivedBytes = recv(clientFd, buffer, BUFFSIZE - 1, 0);
        //while (receivedBytes > 0) {
            //if (receivedBytes == -1) {
                //printf("Response data failed to send: %s\n", strerror(errno));
                //exit(1);
            //}
            //// check if end sequence is found by comparing with last 4 characters in received string
            //if (receivedBytes > 4) {
                //// get last 4 characters of received header 
                //char endSequence[5]; // account for null terminator
                //memset(endSequence, 0, 5);
                //int currBufferPosition = receivedBytes - 1;
                //int startEndSequencePosition = receivedBytes - 6;
                //int endSequenceIndex = 0;
                //while (startEndSequencePosition < currBufferPosition) {
                    //printf("endSequendceIndex: %d\n", endSequenceIndex);
                    //endSequence[endSequenceIndex] = buffer[currBufferPosition];
                    //endSequenceIndex++;
                    //currBufferPosition--;
                //}
                //if (strcmp(endSequence, "\r\n\r\n") == 0) {
                    //break;
                //}
            //}
            //char tempBuffer[BUFFSIZE - (strlen(buffer) + 1)];
            //receivedBytes += recv(clientFd, tempBuffer, BUFFSIZE - 1, 0);
            //strcat(buffer, tempBuffer);
        //}
        //if (receivedBytes == 0) {
            //close(clientFd);
        //} else {
            //// buffer is null terminated
            //strtok(buffer, " "); // get header scheme 
            //strtok(NULL, " "); // get method
            //char *headerPath = strtok(NULL, " ");
            //// remove \r\n\r\n at end of header path 
            //for (int i = 0; i < strlen(headerPath); i++) {
                //if (headerPath[i] == '\r' || headerPath[i] == '\n') {
                    //headerPath[i] = '0';
                //}
            //}

            ////gfserver->handler(&gfcontext, "/courses/ud923/filecorpus/1kb-sample-file-1.html", gfserver->handlerarg);
            //gfserver->handler(&gfcontext, headerPath, gfserver->handlerarg);
            //printf("mesage sent successfully");
        //}


        //free(gfcontext);

        /*
        - receive the request
        - handle the request and send necesary information to the handler (gfcontext_t **, const char *, void*)
        - close client socket connection, cleanup
        */

    }

    freeaddrinfo(serverInfo);
}

