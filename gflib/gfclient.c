
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
  size_t fileLen;
  gfstatus_t status;
  size_t bytesReceived;
};

// optional function for cleaup processing.
void gfc_cleanup(gfcrequest_t **gfr) {
  free(*gfr);
  (*gfr) = NULL;
}

size_t gfc_get_filelen(gfcrequest_t **gfr) {
  // not yet implemented
  size_t fileLen = (*gfr)->fileLen;
  return fileLen;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr) {
  // not yet implemented
  gfstatus_t status = (*gfr)->status;
  return status;
}

gfcrequest_t *gfc_create() {
  // not yet implemented
  gfcrequest_t *newGfcRequest = malloc(sizeof(gfcrequest_t));
  newGfcRequest->headerfunc = NULL;
  newGfcRequest->headerarg = NULL;
  newGfcRequest->writefunc = NULL;
  newGfcRequest->writearg = NULL;
  newGfcRequest->fileLen = 0;
  newGfcRequest->status = GF_ERROR;
  newGfcRequest->bytesReceived = 0;
  return newGfcRequest;
}

size_t gfc_get_bytesreceived(gfcrequest_t **gfr) {
  // not yet implemented
  size_t bytesReceivced = (*gfr)->bytesReceived;
  return bytesReceivced;
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
    freeaddrinfo(serverInfo);
    printf("Request header failed to send: %s\n", strerror(errno));
    return -1;
  }

  // receive header and beginning of content 
  int bufferSize = 2048;
  int freeBufferSpace = bufferSize;
  char *buffer= malloc(bufferSize * sizeof(char)); 
  memset(buffer, 0, freeBufferSpace);
  char *header;
  int totalReceived = 0;
  // first call to recv
  int receivedBytes = recv(socketFd, buffer, freeBufferSpace, 0);
  if (receivedBytes == -1) {
      free(buffer);
      buffer = NULL;
      freeaddrinfo(serverInfo);
      close(socketFd);
      printf("Response header failed to be received: %s\n", strerror(errno));
      return 0;
  }
  if (receivedBytes == 0) {
    printf("server closed prematurely\n");
    free(buffer);
    buffer = NULL;
    freeaddrinfo(serverInfo);
    close(socketFd);
    return 0;
  }
  totalReceived += receivedBytes;
  freeBufferSpace -= receivedBytes;
  printf("free buffer space: %d\n", freeBufferSpace);
  printf("totalReceived: %d\n", receivedBytes);
  printf("starting while loop until we receive header\n");
  // loop until we receive a header, server closes socket, or we don't see \r\n\r\n after 2048 bytes
  while (receivedBytes != 0 && (header = strtok(buffer, "\r\n\r\n")) == NULL && freeBufferSpace != 0) {
    printf("receiving more bytes for header\n");
    int receivedBytes = recv(socketFd, buffer + totalReceived, freeBufferSpace, 0);
    if (receivedBytes == -1) {
      free(buffer);
      buffer = NULL;
      freeaddrinfo(serverInfo);
      close(socketFd);
      printf("Response header failed to be received: %s\n", strerror(errno));
      return 0;
    } else if (receivedBytes == 0) {
      free(buffer);
      buffer = NULL;
      freeaddrinfo(serverInfo);
      close(socketFd);
      printf("server closed prematurely: %s\n", strerror(errno));
      return 0;
    }
    totalReceived += receivedBytes;
    freeBufferSpace -= receivedBytes;
    printf("free buffer space: %d\n", freeBufferSpace);
    printf("totalReceived: %d\n", totalReceived);
  }
  printf("total received bytes (header + begin of content): %d\n", totalReceived);

  // check header
  char *status;
  int fileLength;
  if (header != NULL) {
    if (gfReq->headerfunc != NULL) {
      gfReq->headerfunc(header, strlen(header), gfReq->headerarg);
    } else { //process header
      printf("header: %s\n", header);
      // check scheme is correct and there is a space after it
      if (strncmp(buffer, "GETFILE ", 8) != 0) {
        // header does not start with "GETFILE "
        gfReq->status = GF_INVALID;
        free(buffer);
        buffer = NULL;
        freeaddrinfo(serverInfo);
        close(socketFd);
        return -1;
      }
      // make string with just status and length (removes scheme "GETFILE ")
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
        gfReq->status = GF_INVALID;
        free(buffer);
        buffer = NULL;
        freeaddrinfo(serverInfo);
        close(socketFd);
        return -1;
      }
      status = strtok(statusAndLength, " ");
      if (status == NULL) { // there is no space, thus there is no length
        // we have a status of either FILE_NOT_FOUND, ERROR, or INVALID
        if (strcmp(status, "FILE_NOT_FOUND") == 0) {
          gfReq->status = GF_FILE_NOT_FOUND;
          close(socketFd);
          return 0;
        } else if (strcmp(status, "ERROR") == 0) {
          gfReq->status = GF_ERROR;
          close(socketFd);
          return 0;
        } else if (strcmp(status, "INVALID") == 0) {
          gfReq->status = GF_INVALID;
          close(socketFd);
          return -1;
        } else { // status was invalid
          printf("status invalid\n");
          gfReq->status = GF_INVALID;
          free(buffer);
          buffer = NULL;
          freeaddrinfo(serverInfo);
          close(socketFd);
          return -1;
        }
        printf("status: %s\n", status);
      } else { 
        if (strcmp(status, "FILE_NOT_FOUND") == 0) {
          gfReq->status = GF_FILE_NOT_FOUND;
          free(buffer);
          buffer = NULL;
          freeaddrinfo(serverInfo);
          return 0;
        }
        if (strcmp(status, "ERROR") == 0) {
          gfReq->status = GF_ERROR;
          free(buffer);
          buffer = NULL;
          freeaddrinfo(serverInfo);
          return 0;
        }
        if (strcmp(status, "OK") != 0) { // invalid status
          gfReq->status = GF_INVALID;
          free(buffer);
          buffer = NULL;
          freeaddrinfo(serverInfo);
          return -1;
        }
        gfReq->status = GF_OK;
        char *fileLengthStr = strtok(NULL, " ");
        printf("status: %s\n", status);
        printf("file length str: %s\n", fileLengthStr);
        if (fileLengthStr == NULL) { // file length must be given if status is OK
          printf("file length is not given\n");
          free(buffer);
          buffer = NULL;
          freeaddrinfo(serverInfo);
          return -1;
        }
        // check if file length is a number
        for (int i = 0; i < strlen(fileLengthStr); i++) {
          if (isdigit(fileLengthStr[i]) == 0) {
            printf("fileLengthStr[i]: %c\n", fileLengthStr[i]);
            printf("invalid file length. Not a valid number\n");
            free(buffer);
            buffer = NULL;
            freeaddrinfo(serverInfo);
            return -1;
          }
        }
        fileLength = atoi(fileLengthStr);
        gfReq->fileLen = fileLength;
        printf("file length as int: %d\n", fileLength);
        // insert file length and status in request structure
      }
      // reset recvbuffer and load content into it 
      //char *content = strtok(NULL, "\r\n\r\n");
      //int currContentLen = totalReceived - (strlen(header) + 4);
      //memset(recvBuffer, 0, totalReceived);
      //printf("writing chunk of len: %d\n", currContentLen);
      //if (content != NULL) {
        //strcpy(recvBuffer, content);
        //gfReq->writefunc(recvBuffer, currContentLen, gfReq->writearg);
      //}
    }
  } else {
    gfReq->status = GF_ERROR;
    free(buffer);
    buffer = NULL;
    freeaddrinfo(serverInfo);
    return -1;
  } 

  // processed header, now process content
  printf("\nbeginning processing of content...\n");
  int totalContentBytesRecv = totalReceived - (strlen(header) + 4);
  printf("current content chunk size: %d\n", totalContentBytesRecv);
  printf("total bytes received: %d\n", totalReceived);
  printf("begin of content and 2 chars before content chunk: %c%c%c\n", buffer[strlen(header) + 2], buffer[strlen(header) + 3], buffer[strlen(header) + 4]);
  char content[totalContentBytesRecv];
  printf("strlen(header): %ld\n", strlen(header));
  strncpy(content, buffer + (strlen(header) + 4), (size_t) totalContentBytesRecv);
  //printf("content str: %s\n", content);
  buffer = realloc(buffer, fileLength * sizeof(char)); // make buffer size of file length
  memset(buffer, 0, fileLength * sizeof(char));
  // content may be empty, this could cause an error
  strncpy(buffer, content, (size_t) totalContentBytesRecv);
  printf("buffer: %s\n", buffer);
  gfReq->writefunc(buffer, totalContentBytesRecv, gfReq->writearg);
  printf("writing chunk of size: %d\n", totalContentBytesRecv);
  while (totalContentBytesRecv < fileLength) {
    receivedBytes = recv(socketFd, buffer, fileLength - totalContentBytesRecv, 0);
    if (receivedBytes == -1) {
        close(socketFd);
        printf("file content failed to be received: %s\n", strerror(errno));
        free(buffer);
        buffer = NULL;
        freeaddrinfo(serverInfo);
        exit(1);
    }
    if (receivedBytes == 0) {
      gfReq->bytesReceived = totalReceived;
      free(buffer);
      buffer = NULL;
      freeaddrinfo(serverInfo);
      return -1;
    }
    totalContentBytesRecv += receivedBytes;
    totalReceived += receivedBytes;
    printf("writing chunk of size: %d\n", receivedBytes);
    gfReq->writefunc(buffer, receivedBytes ,gfReq->writearg);
    printf("total content bytes wrote: %d\n", totalContentBytesRecv);
  }

  printf("file length received: %d\n", totalContentBytesRecv);
  printf("file length seen in header: %d\n", fileLength);
  gfReq->bytesReceived = totalContentBytesRecv;
  free(buffer);
  buffer = NULL;
  freeaddrinfo(serverInfo);
  close(socketFd);
  return 0;
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
