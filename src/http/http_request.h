#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "http.h"

extern void http_init_request(http_request_rec *request);

/* release pointer in request
 * this function will not clear non-pointer data
 * therefore, the info about keep-alive mechanism will not disappear
 * */
extern void http_destroy_request(http_request_rec *request);

/* handle request from client
 * include receiving request, parsing request
 *
 * @client: fd/ssl of client
 * @request: struct that stores the info about client request
 * @enable_ssl: if enable ssl, @enable_ssl > 0
 *
 * on success, return 0
 * on timeout error, return -1
 * on parsing error, return -2
 * on other error, return -3
 * */
extern int http_handle_request(union conn client, 
                               http_request_rec *request, 
                               int enable_ssl);

#endif
