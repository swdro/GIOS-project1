#include <string.h>
#include <errno.h>
#include <regex.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "content.h"
#include "gfserver.h"
#include "gfserver-student.h"

#define USAGE                                                                                  \
  "usage:\n"                                                                                   \
  "  gfserver_main [options]\n"                                                                \
  "options:\n"                                                                                 \
  "  -m [content_file]  Content file mapping keys to content filea (Default: 'content.txt')\n" \
  "  -h          		Show this help message.\n"              		                       \
  "  -p [listen_port]   Listen port (Default: 29384)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"content", required_argument, NULL, 'm'},
    {"port", required_argument, NULL, 'p'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv) {
  char *content_map_file = "content.txt";
  gfserver_t *gfs = NULL;
  unsigned short port = 29384;
  int option_char = 0;


  setbuf(stdout, NULL);  // disable caching of standpard output

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "hal:p:m:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 'p':  /* listen-port */
        port = atoi(optarg);
        break;
      case 'h':  /* help */
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
      case 'm':  /* file-path */
        content_map_file = optarg;
        break;
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
    }
  }

  content_init(content_map_file);

  if (port > 65331) {
    fprintf(stderr, "Invalid port number\n");
    exit(EXIT_FAILURE);
  }

  /*Initializing server*/
  gfs = gfserver_create();

  /*Setting options*/
  gfserver_set_handler(&gfs, gfs_handler);
  gfserver_set_port(&gfs, port);
  gfserver_set_maxpending(&gfs, 25);

  /* this implementation does not pass any extra state, so it uses NULL. */
  /* this value could be non-NULL.  You might want to test that in your own */
  /* code. */
  gfserver_set_handlerarg(&gfs, NULL);

  // Run forever
  gfserver_serve(&gfs);
}
