#ifndef HTTP_RESPOND_H
#define HTTP_RESPOND_H

#include <netinet/ip.h>

#include "http.h"
#include "ssl/ez_server.h"

void http_respond(union conn client, 
                  http_request_rec *request, 
                  server_config_rec *sc,
                  struct sockaddr_in *c_addr_ptr,
                  int *status);

#endif
