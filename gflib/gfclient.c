
#include <stdlib.h>

#include "gfclient-student.h"

 // Modify this file to implement the interface specified in
 // gfclient.h.


// optional function for cleaup processing.
void gfc_cleanup(gfcrequest_t **gfr) {}

size_t gfc_get_filelen(gfcrequest_t **gfr) {
  // not yet implemented
  return -1;
}

gfstatus_t gfc_get_status(gfcrequest_t **gfr) {
  // not yet implemented
  return -1;
}

gfcrequest_t *gfc_create() {
  // not yet implemented
  return (gfcrequest_t *)malloc(sizeof(gfcrequest_t) * sizeof(char));
}

size_t gfc_get_bytesreceived(gfcrequest_t **gfr) {
  // not yet implemented
  return -1;
}

void gfc_global_init() {}

void gfc_global_cleanup() {}

int gfc_perform(gfcrequest_t **gfr) {
  // not yet implemented
  return -1;
}

struct gfcrequest_t {
  unsigned short port;
  char *server;
  char *path;
  void (*headerfunc)(void *, size_t, void *);
  void *headerarg;
  void (*writefunc)(void *, size_t, void *);
  void *writearg;
};

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
