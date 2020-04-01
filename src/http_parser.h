#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

extern const char *http_methods[];
extern const char *http_protos[];

/* need to keep this enum same as http_methods at http_parser.c */
enum _http_method {
    GET,
    POST,
    METHOD_AMOUNT
};
typedef enum _http_method http_method;

/* need to keep this enum same as http_protos at http_parser.c */
enum _http_proto {
    HTTP1_1,
    HTTP2,
    HTTP3,
    PROTO_AMOUNT
};
typedef enum _http_proto http_proto;

struct _http_request_rec {
    /* HTTP method */
    http_method method;
    /* the path that client requests */
    char *req_path;
    /* the file extension of the path that client requests 
     * if req_ext == NULL, then there is no extension 
     * if req_ext == -1, then it's a directory */
    char *req_ext;
    /* the HTTP protocol version */
    http_proto proto;
    /* every header will store at a form of "XXX_HEADER=XXX_VALUE" */
    char **headers;
    char *get_data;
    char *post_data;
};
typedef struct _http_request_rec http_request_rec;

/* @req_buf: point to request plaintext 
 * @hrr: point to http_request_rec that will store the parsed information 
 * 
 * if OK, return 0, else on error return -1 */
extern int http_parser(char *req_buf, http_request_rec *hrr);

extern void http_request_rec_destroy(http_request_rec *hrr);

/* @hrr: point http_request_rec
 * @header_name: the header name which is searching for
 * @buf: storing buf
 *
 * if header header_name exists, store its value to buf, and return 0
 * else return -1 */
extern int http_search_header(http_request_rec *hrr, const char *header_name, char *buf);

#endif
