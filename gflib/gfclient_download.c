#include <regex.h>
#include <stdlib.h>

#include "gfclient.h"
#include "workload.h"
#include "gfclient-student.h"

#define BUFSIZE 1024
#define PATH_BUFFER_SIZE 256

#define USAGE                                                             \
  "usage:\n"                                                              \
  "  gfclient_download [options]\n"                                       \
  "options:\n"                                                            \
  "  -w [workload_path]  Path to workload file (Default: workload.txt)\n" \
  "  -p [server_port]    Server port (Default: 29384)\n"                  \
  "  -s [server_addr]    Server address (Default: 127.0.0.1)\n"           \
  "  -h                  Show this help message\n"                        \
  "  -n [num_requests]   Request download total (Default: 14)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"nrequests", required_argument, NULL, 'n'},
    {"workload", required_argument, NULL, 'w'},
    {"port", required_argument, NULL, 'p'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

static void Usage() { fprintf(stdout, "%s", USAGE); }

static void localPath(char *req_path, char *local_path) {
  static int counter = 0;

  snprintf(local_path, PATH_BUFFER_SIZE, "%s_%06d", &req_path[1], counter++);
}

static FILE *openFile(char *path) {
  char *cur, *prev;
  FILE *ans;

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

/* Main ========================================================= */
int main(int argc, char **argv) {
  /* COMMAND LINE OPTIONS ============================================= */

  int option_char = 0;
  gfcrequest_t *gfr;
  FILE *file;
  int nrequests = 15;
  char *workload_path = "workload.txt";

  char *req_path;
  int returncode;
  char local_path[PATH_BUFFER_SIZE];

  char *server = "localhost";
  unsigned short port = 29384;

  setbuf(stdout, NULL);  // disable buffering

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "l:r:hp:s:n:", gLongOptions,
                                    NULL)) != -1) {
    switch (option_char) {
      case 'r':
      case 'n':  // nrequests
        nrequests = atoi(optarg);
        break;	
      case 'h':  // help
        Usage();
        exit(0);
        break;
      case 'p':  // port
        port = atoi(optarg);
        break;
	  case 's':  // server
        server = optarg;
        break;
        Usage();
      case 'w':  // workload-path
        workload_path = optarg;
        break;
      default:
        exit(1);
    }
  }

  if (port > 65331) {
    fprintf(stderr, "Invalid port number\n");
    exit(EXIT_FAILURE);
  }

  if (EXIT_SUCCESS != workload_init(workload_path)) {
    fprintf(stderr, "Unable to load workload file %s.\n", workload_path);
    exit(EXIT_FAILURE);
  }

  gfc_global_init();

  /*Making the requests...*/
  for (int i = 0; i < nrequests; i++) {
    req_path = workload_get_path();

    if (strlen(req_path) > 256) {
      fprintf(stderr, "Request path exceeded maximum of 256 characters\n.");
      exit(EXIT_FAILURE);
    }

    localPath(req_path, local_path);

    file = openFile(local_path);

    gfr = gfc_create();

    gfc_set_port(&gfr, port);
    gfc_set_path(&gfr, req_path);
    gfc_set_server(&gfr, server);


    gfc_set_writefunc(&gfr, writecb);
    gfc_set_writearg(&gfr, file);

    fprintf(stdout, "Requesting %s%s\n", server, req_path);

    if (0 > (returncode = gfc_perform(&gfr))) {
      fprintf(stdout, "gfc_perform returned error %d\n", returncode);
      fclose(file);
      if (0 > unlink(local_path))
        fprintf(stderr, "warning: unlink failed on %s\n", local_path);
    } else {
      fclose(file);
    }

    if (gfc_get_status(&gfr) != GF_OK) {
      if (0 > unlink(local_path))
        fprintf(stderr, "warning: unlink failed on %s\n", local_path);
    }


    fprintf(stdout, "Received:: %zu of %zu bytes\n", gfc_get_bytesreceived(&gfr),
            gfc_get_filelen(&gfr));
        fprintf(stdout, "Status: %s\n", gfc_strstatus(gfc_get_status(&gfr)));

    gfc_cleanup(&gfr);
  }

  gfc_global_cleanup();

  workload_destroy();  // clean up workload package

  return 0;
}
