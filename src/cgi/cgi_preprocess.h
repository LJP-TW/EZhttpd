#ifndef CGI_PREPROCESS_H
#define CGI_PREPROCESS_H

#include <netinet/in.h>

#include "http/http_parser.h"

/* Request Meta-Variables 
 * the order cannot be changed!
 * check RFC-3875 */
enum _rmv_name {
    RMV_AUTH_TYPE,
    RMV_CONTENT_LENGTH,
    RMV_CONTENT_TYPE,
    RMV_GATEWAY_INTERFACE,
    RMV_PATH_INFO,
    RMV_PATH_TRANSLATED,
    RMV_QUERY_STRING,
    RMV_REMOTE_ADDR,
    RMV_REMOTE_HOST,
    RMV_REMOTE_IDENT,
    RMV_REMOTE_USER,
    RMV_REQUEST_METHOD,
    RMV_SCRIPT_NAME,
    RMV_SERVER_NAME,
    RMV_SERVER_PORT,
    RMV_SERVER_PROTOCOL,
    RMV_SERVER_SOFTWARE,
    RMV_NAME_AMOUNT
};
extern const char *RMV_NAME[];

/* Protocol Variables
 * the order cannot be changed!
 * also check RFC-3875 */
enum _pv_name {
    PV_ACCEPT,
    PV_ACCEPT_LANGUAGE,
    PV_COOKIE,
    PV_HOST,
    PV_USER_AGENT,
    PV_NAME_AMOUNT
};
extern const char *PV_NAME[];

struct _cgi_rec {
    /* pass request meta-variables via envp of cgi program */
    char **rmv;
    /* pass request message-body via pipe
     * cgi program can receive this via it's stdin */
    char *rmb;
};
typedef struct _cgi_rec cgi_rec;

/* produce cgi_rec
 * if OK, return 0, else on error return -1 */
int cgi_preprocess(cgi_rec *cgi, 
                   struct sockaddr_in *c_addr, 
                   http_request_rec *hrr);

void cgi_rec_destroy(cgi_rec *cgi);

#endif
