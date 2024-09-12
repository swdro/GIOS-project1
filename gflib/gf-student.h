/*
 *  This file is for use by students to define anything they wish.  It is used by both the gf server and client implementations
 */
#ifndef __GF_STUDENT_H__
#define __GF_STUDENT_H__

#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <netdb.h>
#include <resolv.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <netinet/in.h>

int sendAll(int fd, char *buffer, int bufferLength);

 #endif // __GF_STUDENT_H__
