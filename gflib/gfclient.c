
#include <stdlib.h>

#include "gfclient-student.h"

 // Modify this file to implement the interface specified in
 // gfclient.h.

// max path is 256 + 17 characters for sheme, spaces, method, and end of request marker, and null terminator
#define MAX_REQ_LEN 273
#define RECV_BUFSIZE 2048

struct gfcrequest_t {
  unsigned short port;
  const char *server;
  const char *path;
  void (*headerfunc)(void *, size_t, void *);
  void *headerarg;
  void (*writefunc)(void *, size_t, void *);
  void *writearg;
  char *data;
};

// optional function for cleaup processing.
void gfc_cleanup(gfcrequest_t **gfr) {
  free(*gfr);
}

size_t gfc_get_filelen(gfcrequest_t **gfr) {
  // not yet implemented
  char *requestData = (*gfr)->data;
  if (requestData == NULL) {
    return GF_ERROR;
  }
  // parse get file len
  return -1;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr) {
  // not yet implemented
  char *requestData = (*gfr)->data;
  if (requestData == NULL) {
    return GF_ERROR;
  }
  // parse get status
  return -1;
}

gfcrequest_t *gfc_create() {
  // not yet implemented
  gfcrequest_t *newGfcRequest = malloc(sizeof(gfcrequest_t));
  return newGfcRequest;
}

size_t gfc_get_bytesreceived(gfcrequest_t **gfr) {
  // not yet implemented
  return -1;
}

// don't need to implement
void gfc_global_init() {}

// don't need to implement
void gfc_global_cleanup() {}

int gfc_perform(gfcrequest_t **gfr) {
  // not yet implemented
  gfcrequest_t *gfReq = (*gfr);
  printf("port: %d\n", gfReq->port);
  printf("server: %s\n", gfReq->server);
  printf("path: %s\n", gfReq->path);

  // figure out what to do with header func
  int getAddrInfoStatus;
  struct addrinfo addrInfoHints;
  struct addrinfo *serverInfo;

  // convert portno data type to char array
  memset(&addrInfoHints, 0, sizeof(addrInfoHints));
  addrInfoHints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
  addrInfoHints.ai_socktype = SOCK_STREAM; // TCP stream sockets
  addrInfoHints.ai_flags = AI_PASSIVE;     // assign localhost address to socket structures 
  // perform error checking to look for valid entries in the linked list, see client/server for real examples
  char port[6] = {0}; // port cannot be larger than 5 digits + null terminator
  sprintf(port, "%d", gfReq->port);
  if ((getAddrInfoStatus = getaddrinfo(gfReq->server, port, &addrInfoHints, &serverInfo)) != 0) {
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


  //connect to server
  int connectStatus = connect(socketFd, serverInfo->ai_addr, serverInfo->ai_addrlen);
  if (connectStatus == -1) {
      fprintf(stderr, "error when connecting to server: %s\n", strerror(errno));
      exit(1);
  }

  // construct requestStr
  char requestStr[MAX_REQ_LEN] = {0};
  char *scheme = "GETFILE";
  char *method = "GET";
  char *endRequestMarker = "\r\n\r\n";
  strcat(requestStr, scheme);
  strcat(requestStr, " ");
  strcat(requestStr, method);
  strcat(requestStr, " ");
  strcat(requestStr, gfReq->path);
  strcat(requestStr, endRequestMarker);
  printf("request string: %s, size: %ld\n", requestStr, strlen(requestStr));

  // send request 
  int bytesSent = sendAll(socketFd, requestStr, strlen(requestStr));
  printf("bytes sent: %d\n", bytesSent);
  if (bytesSent == -1) {
    printf("Request header failed to send: %s\n", strerror(errno));
  }

  // receive response
  int totalReceived = 0;
  int receivedBytes = 0;
  int recvBufsize = 2048;
  char *recvBuffer = malloc(recvBufsize * sizeof(char)); 
  memset(recvBuffer, 0, recvBufsize);
  receivedBytes = recv(socketFd, recvBuffer, recvBufsize - 1, 0);
  totalReceived += receivedBytes;
  printf("received bytes: %d\n", receivedBytes);
  printf("total received: %d\n", totalReceived);
  printf("response: %s\n", recvBuffer);
  while (receivedBytes != 0) {
      if (receivedBytes == -1) {
          close(socketFd);
          printf("Response header failed to be received: %s\n", strerror(errno));
          exit(1);
      }
      printf("total received: %d\n", totalReceived);
      printf("buffer size: %d\n", recvBufsize);
      printf("strlen: %ld\n", strlen(recvBuffer));
      int recvBufferFreeSpace = recvBufsize - (totalReceived + 1);
      printf("free space in buffer: %d\n", recvBufferFreeSpace);
      if (recvBufferFreeSpace == 0) {
        recvBufferFreeSpace += recvBufsize;
        recvBufsize *= 2;
        recvBuffer = realloc(recvBuffer, recvBufsize * sizeof(char));
      }
      //memset(buffer, 0, BUFSIZE);
      receivedBytes = recv(socketFd, recvBuffer + totalReceived, recvBufferFreeSpace, 0);
      totalReceived += receivedBytes;
      printf("bytes received: %d\n", receivedBytes);
      printf("total bytes available in buffer: %d\n", recvBufferFreeSpace);
      printf("total bytes received: %d\n", totalReceived);
      recvBuffer[totalReceived] = 0;
      printf("response: %s\n", recvBuffer);
  }

  // validate response
  /*
  SCHEME - always GETFILE
  STATUS - can be OK, FILE_NOT_FOUND, ERROR, or INVALID
  LENGTH - only available if status == OK
  END OF HEADER MARKER - always \r\n\r\n
  CONTENT - none if status is FILE_NOT_FOUND or ERROR

  separate on \r\n\r\n so now we have header, content
  - check first 8 characters in header are "GETFILE "
  - get next str on condition: we hit null terminator, or we hit a space. Then check the status is valid
  - if status != ok, we should not have length. Otherwise we read the length and make sure it's a number
  
  - for content we write to file
  */
  // split response header on "\r\n\r\n"
  char *header = strtok(recvBuffer, "\r\n\r\n");
  if (header == NULL) {
    // invalid header, does not have "\r\n\r\n"
    return -1;
  }
  char *content = strtok(NULL, "\r\n\r\n");

  printf("header: %s\n", header);
  printf("content: %s\n", content);

  // check scheme is correct and there is a space after it
  if (strncmp(recvBuffer, "GETFILE ", 8) != 0) {
    // header does not start with "GETFILE "
    return -1;
  }

  printf("strlen(header): %ld\n", strlen(header));
  printf("strlen(header + 8): %ld\n", strlen(header + 8));
  int statusAndLengthLen = strlen(header + 8) + 1; // subtract 8 since we don't need scheme and space, also account for null term
  printf("statusAndLengthLen: %d\n", statusAndLengthLen);
  char statusAndLength[statusAndLengthLen];  
  memset(statusAndLength, 0, statusAndLengthLen);
  printf("header[8]: %c\n", header[8]);
  strncpy(statusAndLength, &header[8], statusAndLengthLen - 1); // account or null term
  //statusAndLength[statusAndLengthLen] = '0';
  printf("header without scheme: %s\n", statusAndLength);

  // check if header without scheme is at least 4 characters, if OK we must have at least "OK n", otherwise it will be longer
  if (strlen(statusAndLength) < 4) {
    // invalid length of status and length
    return -1;
  }

  char *status = strtok(statusAndLength, " ");
  if (status == NULL) { // there is no space, thus there is no length
    if (strcmp(status, "FILE_NOT_FOUND") == 0 || strcmp(status, "ERROR") == 0 || strcmp(status, "INVALID") == 0) {
      // we have a status of either FILE_NOT_FOUND, ERROR, or INVALID
      printf("status: %s\n", status);
    } else {
      // invalid status
      printf("status invalid\n");
      return -1;
    }
  } else { 
    if (strcmp(status, "OK") != 0) { // invalid status
      return -1;
    }
    char *fileLengthStr = strtok(NULL, " ");
    printf("file length str: %s\n", fileLengthStr);
    if (fileLengthStr == NULL) { // file length must be given if status is OK
      printf("file length is not given\n");
      return -1;
    }
    // check if file length is a number
    for (int i = 0; i < strlen(fileLengthStr); i++) {
      if (isdigit(fileLengthStr[i]) != 0) {
        printf("fileLengthStr[i]: %c\n", fileLengthStr[i]);
        printf("invalid file length. Not a valid number\n");
        return -1;
      }
    }
    int fileLength = atoi(fileLengthStr);
    printf("file length: %d\n", fileLength);
    printf("status: %s\n", status);
    // insert file length and status in request structure
  }

  close(socketFd);

  return -1;
}


void gfc_set_headerarg(gfcrequest_t **gfr, void *headerarg) {
  (*gfr)->headerarg = headerarg;
}

void gfc_set_port(gfcrequest_t **gfr, unsigned short port) {
  (*gfr)->port = port;
}

void gfc_set_headerfunc(gfcrequest_t **gfr, void (*headerfunc)(void *, size_t, void *)) {
  (*gfr)->headerfunc = headerfunc;
}

void gfc_set_server(gfcrequest_t **gfr, const char *server) {
  (*gfr)->server = server;
}

void gfc_set_writearg(gfcrequest_t **gfr, void *writearg) {
  (*gfr)->writearg = writearg;
}

void gfc_set_path(gfcrequest_t **gfr, const char *path) {
  (*gfr)->path = path;
}

void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void *, size_t, void *)) {
  (*gfr)->writefunc = writefunc;
}

const char *gfc_strstatus(gfstatus_t status) {
  const char *strstatus = "UNKNOWN";

  switch (status) {

    case GF_OK: {
      strstatus = "OK";
    } break;

    case GF_FILE_NOT_FOUND: {
      strstatus = "FILE_NOT_FOUND";
    } break;

   case GF_INVALID: {
      strstatus = "INVALID";
    } break;
   
   case GF_ERROR: {
      strstatus = "ERROR";
    } break;

  }

  return strstatus;
}
