#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "http.h"

/* @req_buf: point to request plaintext 
 * @hrr: point to http_request_rec that will store the parsed information 
 * 
 * if OK, return 0, else on error return -1 */
extern int http_parser(char *req_buf, http_request_rec *hrr);

/* @hrr: point http_request_rec
 * @header_name: the header name which is searching for
 * @buf: storing buf
 *
 * if header header_name exists, store its value to buf, and return 0
 * else return -1 */
extern int http_search_header(http_request_rec *hrr, const char *header_name, char *buf);

#endif
