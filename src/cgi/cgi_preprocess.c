#include <string.h>
#include <arpa/inet.h>

#include "cgi_preprocess.h"
#define BUF_MAX 0x1000

/* the order cannot be changed! */
const char *RMV_NAME[] = {"AUTH_TYPE", 
                          "CONTENT_LENGTH",
                          "CONTENT_TYPE",
                          "GATEWAY_INTERFACE",
                          "PATH_INFO", 
                          "PATH_TRANSLATED", 
                          "QUERY_STRING", 
                          "REMOTE_ADDR", 
                          "REMOTE_HOST", 
                          "REMOTE_IDENT", 
                          "REMOTE_USER", 
                          "REQUEST_METHOD", 
                          "SCRIPT_NAME", 
                          "SERVER_NAME", 
                          "SERVER_PORT", 
                          "SERVER_PROTOCOL", 
                          "SERVER_SOFTWARE"};

/* the order cannot be changed! */
const char *PV_NAME[] = {"ACCEPT",
                         "ACCEPT_LANGUAGE",
                         "COOKIE",
                         "HOST",
                         "USER_AGENT"};

int cgi_preprocess(cgi_rec *cgi, 
                   struct sockaddr_in *c_addr, 
                   http_request_rec *hrr)
{
    int i = 0, j = 0;
    char str[BUF_MAX] = { 0 };
    char tmp[BUF_MAX] = { 0 };

#ifdef DEBUG
    printf("[#] cgi_preprocess\n");
#endif

    cgi->rmv = malloc(sizeof(char *) * (RMV_NAME_AMOUNT + PV_NAME_AMOUNT + 1));
    
    /* TODO: handle AUTH_TYPE
     * for now, just keep it empty string */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    cgi->rmv[i] = strdup(str);
    ++i;

    /* handle CONTENT_LENGTH */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    if (!http_search_header(hrr, "CONTENT-LENGTH", tmp)) {
        /* find it in headers */
        strcat(str, tmp);
    }
    cgi->rmv[i] = strdup(str);
    ++i;

    /* handle CONTENT_TYPE */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    if (!http_search_header(hrr, "CONTENT-TYPE", tmp)) {
        /* find it in headers */
        strcat(str, tmp);
    }
    cgi->rmv[i] = strdup(str);
    ++i;

    /* handle GATEWAY_INTERFACE */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=CGI/1.1");
    cgi->rmv[i] = strdup(str);
    ++i;

    /* TODO: handle PATH_INFO */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    cgi->rmv[i] = strdup(str);
    ++i;

    /* TODO: handle PATH_TRANSLATED */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    cgi->rmv[i] = strdup(str);
    ++i;
#ifdef DEBUG
    if (hrr->get_data != NULL) 
        printf("[#] cgi_preprocess: %s\n", hrr->get_data);
#endif

    /* handle QUERY_STRING */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    if (hrr->get_data != NULL) {
        /* find it in headers */
        strcat(str, hrr->get_data);
    }
    cgi->rmv[i] = strdup(str);
    ++i;

    /* handle REMOTE_ADDR */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    strcat(str, inet_ntoa(c_addr->sin_addr));
    cgi->rmv[i] = strdup(str);
    ++i;

    /* TODO: handle REMOTE_HOST */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    cgi->rmv[i] = strdup(str);
    ++i;

    /* handle REMOTE_IDENT */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    cgi->rmv[i] = strdup(str);
    ++i;

    /* TODO: handle REMOTE_USER */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    cgi->rmv[i] = strdup(str);
    ++i;

    /* handle REQUEST_METHOD */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    strcat(str, http_methods[hrr->method]);
    cgi->rmv[i] = strdup(str);
    ++i;

    /* TODO: handle SCRIPT_NAME */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    cgi->rmv[i] = strdup(str);
    ++i;

    /* TODO: handle SERVER_NAME */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    cgi->rmv[i] = strdup(str);
    ++i;

    /* TODO: handle SERVER_PORT */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    cgi->rmv[i] = strdup(str);
    ++i;

    /* TODO: handle SERVER_PROTOCOL */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    cgi->rmv[i] = strdup(str);
    ++i;

    /* TODO: handle SERVER_SOFTWARE */
    strcpy(str, RMV_NAME[i]);
    strcat(str, "=");
    cgi->rmv[i] = strdup(str);
    ++i;

    /* handle protocol variables */
    /* TODO: ignore case-sensitive */
    for (j = 0; j < PV_NAME_AMOUNT; ++j) {
        strcpy(str, "HTTP_");
        strcat(str, PV_NAME[j]);
        strcat(str, "=");
        if (!http_search_header(hrr, PV_NAME[j], tmp)) {
            /* find it in headers */
            strcat(str, tmp);
        }
        cgi->rmv[i + j] = strdup(str);
    }

    cgi->rmv[i + j] = NULL;

    if (hrr->post_data)
        cgi->rmb = strdup(hrr->post_data);
    else
        cgi->rmb = NULL;

    return 0;
}

void cgi_rec_destroy(cgi_rec *cgi)
{
    int i = 0;

    if (cgi->rmv) {
        while (cgi->rmv[i] != NULL)
            free(cgi->rmv[i++]);
        free(cgi->rmv);
    }

    if (cgi->rmb)
        free(cgi->rmb);
}
