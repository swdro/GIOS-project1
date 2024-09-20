/*
 *  This file is for use by students to define anything they wish.  It is used by the gf server implementation
 */
#ifndef __GF_SERVER_STUDENT_H__
#define __GF_SERVER_STUDENT_H__

#include "gf-student.h"
#include "gfserver.h"
#include "content.h"
#include <stdlib.h>
#include <pthread.h>
#include "steque.h"


void init_threads(size_t numthreads, steque_t *queue);
void cleanup_threads();

#endif // __GF_SERVER_STUDENT_H__
