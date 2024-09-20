#include "gfserver-student.h"
#include "gfserver.h"
#include "workload.h"
#include "content.h"

pthread_t *thread_id_arr;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c_cons = PTHREAD_COND_INITIALIZER;

typedef struct {
	gfcontext_t *ctx;
	const char *path;
} request_data_t;

//
//  The purpose of this function is to handle a get request
//
//  The ctx is a pointer to the "context" operation and it contains connection state
//  The path is the path being retrieved
//  The arg allows the registration of context that is passed into this routine.
//  Note: you don't need to use arg. The test code uses it in some cases, but
//        not in others.
//
gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void* arg){
	printf("handler is being called\n");
	// initialize queue
	steque_t *queue = (steque_t *) arg;

	// initialize request data DS
	request_data_t *rd = (request_data_t *) malloc(sizeof(request_data_t)); // make sure you free!
	rd->ctx = *ctx;
	rd->path = path; // we may need to copy this

	// get lock to enqueue
	pthread_mutex_lock(&queue_mutex);
		// add to queue
		steque_enqueue(queue, rd);
	pthread_mutex_unlock(&queue_mutex);
	pthread_cond_broadcast(&c_cons);

	// set ctx to null
	*ctx = NULL;

	return gfh_success;
}

/*
we may neet to add addditional INITIALIZATION or WORKER FUNCTIONS to this file. Ex. main file will need a way to set number of threads
Use extern keyword to make these functions available in gfserver_main.c

Use steque.ch for the boss/work queue and use at least one mutex and one condition variable. 

INSTRUCTIONS:
	- initialize pool of workers before serving
	- each worker runs in endless loop waiting for a request
	- remember to set (*ctx) = null!!!!
	- also we have to copy context
	- make sure you free request_data_t and the gfcontext_t it's storing (not sure about freeing gfcontext_t, try freeing request_data_t first)
	- only pop if queue is not empty
	- destroy mutex and condition variable
*/

void *handle_request_worker(void *arg) {
	steque_t *queue = (steque_t *) arg;
	while (1) {
		// get mutex to accesss queue
		pthread_mutex_lock(&queue_mutex);
			while (steque_isempty(queue)) {
				pthread_cond_wait(&c_cons, &queue_mutex);
			}
			request_data_t *rd = steque_pop(queue);
		pthread_mutex_unlock(&queue_mutex);
		
		printf("handling request in thread: %lu\n", pthread_self());

		// make sure path is valid
		int content_fd = content_get(rd->path);
		if (content_fd == -1) { // doesn't exist
			printf("file descriptor not found: %d\n", content_fd);
			gfs_sendheader(&(rd->ctx), GF_FILE_NOT_FOUND, 0);
		}
		printf("file descriptor of content: %d\n", content_fd);

		// get file information
		struct stat file_info;
		if (fstat(content_fd, &file_info)) {
			perror("failed to call fstat\n");
			exit(gfh_failure);
		}
		long int file_size = file_info.st_size;
		printf("file size: %ld\n", file_size);

		gfs_sendheader(&(rd->ctx), GF_OK, file_size);

		int bytes_read = 0;
		int total_bytes_read = 0;
		int read_buffer_len = 512;
		char *read_buffer[read_buffer_len];
		while ((bytes_read = pread(content_fd, read_buffer, read_buffer_len, total_bytes_read)) != 0) {
			int total_bytes_sent = 0;
			while (total_bytes_sent != bytes_read) {
				int bytes_sent = gfs_send(&(rd->ctx), read_buffer, bytes_read);
				total_bytes_sent += bytes_sent;
			}
			total_bytes_read += bytes_read;
			printf("total bytes read: %d\n", total_bytes_read);
		}
		//free(rd->ctx);
		free(rd);
	}

	return NULL;
}

void init_threads(size_t numthreads, steque_t *queue) {
	thread_id_arr = malloc(sizeof(pthread_t) * numthreads);
	for (int i = 0; i < numthreads; i++) {
		pthread_create(&thread_id_arr[i], NULL, handle_request_worker, queue);
	}

}

void cleanup_threads() {
	free(thread_id_arr);
	int mutex_destroy_status = pthread_mutex_destroy(&queue_mutex);
	if (mutex_destroy_status != 0) {
		perror("error destroying mutex\n");
	}
	int cond_var_destroy_status = pthread_cond_destroy(&c_cons);
	if (cond_var_destroy_status != 0) {
		perror("error destroying mutex\n");
	}
}
