#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <unistd.h>

#include "http_request.h"

#define MAX_BUF_LEN 0x10000

void http_init_request(http_request_rec *request)
{
    request->first_req = 1;
    request->keep_alive = 0;
    request->tv.tv_sec = HTTP_DEFAULT_TIMEOUT;
    request->tv.tv_usec = 0;
}

void http_destroy_request(http_request_rec *request)
{
    int i = 0;
#ifdef DEBUG
    printf("[D] destroy request\n");
#endif
    if (request->req_path)
        free(request->req_path);
    if (request->req_ext != NULL && request->req_ext != (char *)-1)
        free(request->req_ext);

    if (request->headers) {
        while (request->headers[i] != NULL) {
            free(request->headers[i++]);
        }
        free(request->headers);
    }

    if (request->get_data)
        free(request->get_data);

    if (request->post_data)
        free(request->post_data);
#ifdef DEBUG
    printf("[D] destroy request OK\n");
#endif
}

int http_handle_request(int cfd, http_request_rec *request)
{
    fd_set fds, read_fds;
    int ret;
    char rbuf[MAX_BUF_LEN];

    FD_ZERO(&fds);
    FD_SET(cfd, &fds);
    read_fds = fds;

    /* check whether timeout */
    ret = select(FD_SETSIZE, &read_fds, NULL, NULL, &request->tv);

    if (ret < 0) {
        fprintf(stderr, "select error\n");
        return -3;
    } else if (ret == 0) {
        fprintf(stderr, "client timeout\n");
        return -1;
    }

    ret = recv(cfd, rbuf, MAX_BUF_LEN, 0);

    if (ret == -1) {
        fprintf(stderr, "recv error\n");
        return -3;
    } else if (ret == 0) {
        fprintf(stderr, "socket peer has performed an orderly shutdown\n");
        return -3;
    }

    rbuf[ret] = '\0';

    /* handle request header */
    if (http_parser(rbuf, request) == -1) {
        fprintf(stderr, "error during parsing the request\n");
        return -2;
    }

    if (request->first_req) {
        request->first_req = 0;
        ret = http_search_header(request, "Connection", rbuf);
        if (ret == 0 && strncasecmp(rbuf, "keep-alive", 10) == 0) {
            request->keep_alive = 1;
            request->keep_alive_amount = HTTP_KEEP_ALIVE_MAX;
            request->tv.tv_sec = HTTP_KEEP_ALIVE_TIMEOUT;
            request->tv.tv_usec = 0;
        }
    }

    return 0;
}
