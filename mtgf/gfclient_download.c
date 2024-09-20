#include <stdlib.h>

#include "gfclient-student.h"

#define MAX_THREADS 1024
#define PATH_BUFFER_SIZE 512

#define USAGE                                                             \
  "usage:\n"                                                              \
  "  gfclient_download [options]\n"                                       \
  "options:\n"                                                            \
  "  -s [server_addr]    Server address (Default: 127.0.0.1)\n"           \
  "  -t [nthreads]       Number of threads (Default 8 Max: 1024)\n"       \
  "  -p [server_port]    Server port (Default: 17394)\n"                  \
  "  -w [workload_path]  Path to workload file (Default: workload.txt)\n" \
  "  -h                  Show this help message\n"                        \
  "  -n [num_requests]   Request download total (Default: 16)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"port", required_argument, NULL, 'p'},
    {"help", no_argument, NULL, 'h'},
    {"server", required_argument, NULL, 's'},
    {"nthreads", required_argument, NULL, 't'},
    {"workload", required_argument, NULL, 'w'},
    {"nrequests", required_argument, NULL, 'n'},
    {NULL, 0, NULL, 0}};

/* THREAD VARIABLES ======================================================== */
pthread_t *thread_pool;
typedef struct {
  steque_t * queue;
  char *path;
  int port;
  char *server;
} request_data_t;

static void Usage() { fprintf(stderr, "%s", USAGE); }

static void localPath(char *req_path, char *local_path) {
  static int counter = 0;

  sprintf(local_path, "%s-%06d", &req_path[1], counter++);
}

static FILE *openFile(char *path) {
  FILE *ans;
  char *cur, *prev;

  /* Make the directory if it isn't there */
  prev = path;
  while (NULL != (cur = strchr(prev + 1, '/'))) {
    *cur = '\0';

    if (0 > mkdir(&path[0], S_IRWXU)) {
      if (errno != EEXIST) {
        perror("Unable to create directory");
        exit(EXIT_FAILURE);
      }
    }

    *cur = '/';
    prev = cur;
  }

  if (NULL == (ans = fopen(&path[0], "w"))) {
    perror("Unable to open file");
    exit(EXIT_FAILURE);
  }

  return ans;
}


/* Callbacks ========================================================= */
static void writecb(void *data, size_t data_len, void *arg) {
  FILE *file = (FILE *)arg;
  fwrite(data, 1, data_len, file);
}

void worker_thread(void *arg) {
  steque_t * queue = (steque_t *) queue;
  int returncode = 0;

  /* Note that when you have a worker thread pool, you will need to move this
    * logic into the worker threads */
  char *req_path = workload_get_path();

  if (strlen(req_path) > PATH_BUFFER_SIZE) {
    fprintf(stderr, "Request path exceeded maximum of %d characters\n.", PATH_BUFFER_SIZE);
    exit(EXIT_FAILURE);
  }

  char local_path[PATH_BUFFER_SIZE];
  localPath(req_path, local_path);

  FILE *file = NULL;
  file = openFile(local_path);

  gfr = gfc_create();
  gfc_set_path(&gfr, req_path);

  gfc_set_port(&gfr, port);
  gfc_set_server(&gfr, server);
  gfc_set_writearg(&gfr, file);
  gfc_set_writefunc(&gfr, writecb);



  fprintf(stdout, "Requesting %s%s\n", server, req_path);

  if (0 > (returncode = gfc_perform(&gfr))) {
    fprintf(stdout, "gfc_perform returned an error %d\n", returncode);
    fclose(file);
    if (0 > unlink(local_path))
      fprintf(stderr, "warning: unlink failed on %s\n", local_path);
  } else {
    fclose(file);
  }

  if (gfc_get_status(&gfr) != GF_OK) {
    if (0 > unlink(local_path)) {
      fprintf(stderr, "warning: unlink failed on %s\n", local_path);
    }
  }

  fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(&gfr)));
  fprintf(stdout, "Received %zu of %zu bytes\n", gfc_get_bytesreceived(&gfr),
          gfc_get_filelen(&gfr));

  gfc_cleanup(&gfr);

  /*
    * note that when you move the above logic into your worker thread, you will
    * need to coordinate with the boss thread here to effect a clean shutdown.
    */
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  /* COMMAND LINE OPTIONS ============================================= */
  int returncode = 0;
  char *workload_path = "workload.txt";
  int option_char = 0;
  char *server = "localhost";
  unsigned short port = 17394;


  char *req_path = NULL;
  int nthreads = 9;
  char local_path[PATH_BUFFER_SIZE];
  int nrequests = 13;

  gfcrequest_t *gfr = NULL;
  FILE *file = NULL;

  setbuf(stdout, NULL);  // disable caching

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:n:hs:t:r:w:", gLongOptions,
                                    NULL)) != -1) {
    switch (option_char) {

      case 'p':  // port
        port = atoi(optarg);
        break;
      case 's':  // server
        server = optarg;
        break;
      case 'r': // nrequests
      case 'n': // nrequests
        nrequests = atoi(optarg);
        break;
      case 'w':  // workload-path
        workload_path = optarg;
        break;
      case 't':  // nthreads
        nthreads = atoi(optarg);
        break;
      case 'h':  // help
        Usage();
        exit(0);
      default:
        Usage();
        exit(1);
    }
  }

  if (EXIT_SUCCESS != workload_init(workload_path)) {
    fprintf(stderr, "Unable to load workload file %s.\n", workload_path);
    exit(EXIT_FAILURE);
  }
  if (port > 65331) {
    fprintf(stderr, "Invalid port number\n");
    exit(EXIT_FAILURE);
  }
  if (nthreads < 1 || nthreads > MAX_THREADS) {
    fprintf(stderr, "Invalid amount of threads\n");
    exit(EXIT_FAILURE);
  }
  gfc_global_init();

  // add your threadpool creation here
  /*
    pool of threads initialized based on command line
    boss thread should enqueue specified number of requests. Once boss confirms all requests have been completed, terminate worker threads and exit.
    Use steque.ch for the work queue and use at least one mutex and one condition variable. 
  */
  steque_t *queue;
  steque_init(queue);
	thread_pool = malloc(sizeof(pthread_t) * nthreads);
	for (int i = 0; i < nthreads; i++) {
    request_data_t *rd = malloc(sizeof(request_data_t));
    rd->path = req_path;
    rd->port = port;
    rd->server = server;
    rd->queue = queue;
		pthread_create(&thread_pool[i], NULL, worker_thread, rd);
	}

  /* Build your queue of requests here */
  for (int i = 0; i < nrequests; i++) {
    
  }

  gfc_global_cleanup();  /* use for any global cleanup for AFTER your thread
                          pool has terminated. */

  return 0;
}
