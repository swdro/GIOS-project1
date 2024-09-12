/*
 *  This file is for use by students to define anything they wish.  It is used by both the gf server and client implementations
 */

#include "gf-student.h"


int sendAll(int fd, char *buffer, int bufferLength) {
    int totalBytesSent = 0; // all bytes sent
    int totalBytesLeft = bufferLength;
    int bytesSent; // bytes sent on current iteration

    // send entire message
    while (totalBytesSent < bufferLength) {
        bytesSent = send(fd, buffer + totalBytesSent, totalBytesLeft, 0);
        if (bytesSent == -1) { break; }
        totalBytesSent += bytesSent;
        totalBytesLeft -= bytesSent;
    }
    return bytesSent;
}

