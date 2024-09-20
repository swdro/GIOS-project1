#include <stdlib.h>

#include "gfserver-student.h"

#define USAGE                                                                                     \
  "usage:\n"                                                                                      \
  "  gfserver_main [options]\n"                                                                   \
  "options:\n"                                                                                    \
  "  -m [content_file]   Content file mapping keys to content files (Default: content.txt\n"      \
  "  -p [listen_port]    Listen port (Default: 17394)\n"                                          \
  "  -h                  Show this help message.\n"                                               \
  "  -t [nthreads]       Number of threads (Default: 16)\n"                                       \
  "  -d [delay]          Delay in content_get, default 0, range 0-5000000 "                       \
  "(microseconds)\n "

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"delay", required_argument, NULL, 'd'},
    {"port", required_argument, NULL, 'p'},
    {"nthreads", required_argument, NULL, 't'},
    {"content", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

extern unsigned long int content_delay;

extern gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void *arg);

static void _sig_handler(int signo) {
  if ((SIGINT == signo) || (SIGTERM == signo)) {
    exit(signo);
  }
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  char *content_map = "content.txt";
  int option_char = 0;
  gfserver_t *gfs = NULL;
  int nthreads = 14;
  unsigned short port = 17394;

  setbuf(stdout, NULL);

  if (SIG_ERR == signal(SIGINT, _sig_handler)) {
    fprintf(stderr, "Can't catch SIGINT...exiting.\n");
    exit(EXIT_FAILURE);
  }

  if (SIG_ERR == signal(SIGTERM, _sig_handler)) {
    fprintf(stderr, "Can't catch SIGTERM...exiting.\n");
    exit(EXIT_FAILURE);
  }

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:d:rhm:t:", gLongOptions,
                                    NULL)) != -1) {
    switch (option_char) {
      case 'h':  /* help */
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
      case 'd':  /* delay */
        content_delay = (unsigned long int)atoi(optarg);
        break;
	  case 'p':  /* listen-port */
        port = atoi(optarg);
        break;
      case 'm':  /* file-path */
        content_map = optarg;
        break;
      case 't':  /* nthreads */
        nthreads = atoi(optarg);
        break;
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);


    }
  }

  /* not useful, but it ensures the initial code builds without warnings */
  if (nthreads < 1) {
    nthreads = 1;
  }

  if (content_delay > 5000000) {
    fprintf(stderr, "Content delay must be less than 5000000 (microseconds)\n");
    exit(__LINE__);
  }

  content_init(content_map);

  /* Initialize thread management */


  steque_t *request_queue = malloc(sizeof(steque_t));
  steque_init(request_queue);
  init_threads(nthreads, request_queue);

  /*Initializing server*/
  gfs = gfserver_create();

  //Setting options
  gfserver_set_port(&gfs, port);
  gfserver_set_maxpending(&gfs, 24);
  gfserver_set_handler(&gfs, gfs_handler);
  gfserver_set_handlerarg(&gfs, request_queue);  // doesn't have to be NULL!

  /*Loops forever*/
  gfserver_serve(&gfs);

  /*not sure if necessary*/
  cleanup_threads();
  steque_destroy(request_queue);
  free(request_queue);
}
