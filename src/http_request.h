#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include "http.h"
#include "http_parser.h"

extern void http_init_request(http_request_rec *request);

/* release pointer in request
 * this function will not clear non-pointer data
 * therefore, the info about keep-alive mechanism will not disappear
 * */
extern void http_destroy_request(http_request_rec *request);

/* handle request from client
 * include receiving request, parsing request
 *
 * @cfd: fd of client socket
 * @tv: timeout variable
 * @request: struct that stores the info about client request
 *
 * on success, return 0
 * on timeout error, return -1
 * on parsing error, return -2
 * on other error, return -3
 * */
extern int http_handle_request(int cfd, http_request_rec *request);

#endif
