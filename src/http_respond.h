#ifndef HTTP_RESPOND_H
#define HTTP_RESPOND_H
#include <sys/socket.h>
#include <netinet/ip.h>

#include "http.h"

void http_respond(int cfd, 
                  http_request_rec *request, 
                  char *root, 
                  struct sockaddr_in *c_addr_ptr,
                  int *status);

#endif
