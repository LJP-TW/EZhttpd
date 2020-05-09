#ifndef HTTP_H
#define HTTP_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/time.h>

/* need to keep this enum same as http_methods at http_parser.c */
extern const char *http_methods[];
enum _http_method {
    GET,
    POST,
    METHOD_AMOUNT
};
typedef enum _http_method http_method;

/* need to keep this enum same as http_protos at http_parser.c */
extern const char *http_protos[];
enum _http_proto {
    HTTP1_1,
    HTTP2,
    HTTP3,
    PROTO_AMOUNT
};
typedef enum _http_proto http_proto;

#define HTTP_DEFAULT_TIMEOUT 5
#define HTTP_KEEP_ALIVE_MAX 100
#define HTTP_KEEP_ALIVE_MAX_STR "100"
#define HTTP_KEEP_ALIVE_TIMEOUT 10
#define HTTP_KEEP_ALIVE_TIMEOUT_STR "10"

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
    /* the amount of requests that a one TCP connection could send */
    int keep_alive_amount;
    /* the timeout of keep-alive mechanism */
    struct timeval tv;
    /* is this the first time that the request is handled */
    unsigned char first_req:1;
    /* keep-alive flag */
    unsigned char keep_alive:1;
};
typedef struct _http_request_rec http_request_rec;

union conn {
    int cfd;
    SSL *ssl;
};

#endif
