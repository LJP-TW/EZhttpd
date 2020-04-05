#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "http_respond.h"
#include "cgi_preprocess.h"

#define WRITE(X) write(cfd, X, strlen(X))
#define WRITE_CONTENT_LENGTH(SIZE) \
    WRITE(CONTENT_LENGTH); \
    WRITE(SIZE); \
    WRITE(CR_LF);
#define HTTP_KEEP_ALIVE(TIMEOUT, MAX) \
    "Keep-Alive: timeout=" TIMEOUT ", max=" MAX "\r\n"

const char STATE_200[] = " 200 Ok\r\n";
const char STATE_404[] = " 404 Not Found\r\n";
const char STATE_500[] = " 500 Internal Server Error\r\n";

const char SERVER[] = "Server: ezhttpd\r\n";
const char CONTENT_TYPE_HTML[] = "Content-Type: text/html\r\n";
const char CONTENT_TYPE_PLAIN[] = "Content-Type: text/plain\r\n";
const char CONNECTION_KEEP_ALIVE[] = "Connection: keep-alive\r\n";
const char KEEP_ALIVE[] = HTTP_KEEP_ALIVE(HTTP_KEEP_ALIVE_TIMEOUT_STR, 
                                          HTTP_KEEP_ALIVE_MAX_STR);
const char CONTENT_LENGTH[] = "Content-Length: ";

const char CR_LF[] = "\r\n";

const char BODY_404[] = "<h1>Not Found, 88888</h1>";
const char BODY_500[] = "<h1>Internal Server Error</h1>";

static inline
char* complete_path(http_request_rec *request, char *root)
{
    char *path;
    int len;

    len = strlen(root) + strlen(request->req_path) + 1;
    path = malloc(len);
    memset(path, 0, len);
    strcat(path, root);
    strcat(path, request->req_path);

    return path;
}

static inline
void http_send_headers(int cfd,
                       http_request_rec *request, 
                       const char *proto, 
                       const char *state,
                       const char *content_type,
                       const char *content_length)
{
    WRITE(proto);
    WRITE(state);
    WRITE(SERVER);

    if (content_type)
        WRITE(content_type);

    if (request->keep_alive) {
        WRITE(CONNECTION_KEEP_ALIVE);
        // WRITE(KEEP_ALIVE);
    }

    if (content_length)
        WRITE_CONTENT_LENGTH(content_length);
}

void http_respond(int cfd, 
                  http_request_rec *request, 
                  char *root, 
                  struct sockaddr_in *c_addr_ptr,
                  int *status)
{
    const char *proto;
    char *path;
    char *s_fsize;
    long fsize;

    proto = http_protos[request->proto]; 
    path = complete_path(request, root);

    if (request->req_ext == (char *)-1) {
        /* path is a directory */
        /* TODO: list directory */
        s_fsize = malloc(0x70);
        snprintf(s_fsize, 0x70, "%ld", strlen(BODY_404));

        http_send_headers(cfd, 
                          request, 
                          proto, 
                          STATE_404, 
                          CONTENT_TYPE_HTML,
                          s_fsize);
        WRITE(CR_LF);
        WRITE(BODY_404);

        free(s_fsize);
        *status = 404;
        goto RESPOND_EXIT;
    }

    if (access(path, F_OK) != -1) {
        if (!strncmp(request->req_ext, "html", 4)) {
            FILE *fp;
            char *fcontent;

            fp = fopen(path, "rb");
            if (fp == NULL)
                goto NOT_FOUND;

            fseek(fp, 0, SEEK_END);
            fsize = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            s_fsize = malloc(0x70);
            snprintf(s_fsize, 0x70, "%ld", fsize);

            /* read path */
            fcontent = malloc(fsize + 1);
            fread(fcontent, 1, fsize, fp);
            fclose(fp);

            fcontent[fsize] = 0;

            http_send_headers(cfd, 
                              request, 
                              proto, 
                              STATE_200, 
                              CONTENT_TYPE_HTML,
                              s_fsize);
            WRITE(CR_LF);
            write(cfd, fcontent, fsize);

            free(s_fsize);
            free(fcontent);
            *status = 200;
            goto RESPOND_EXIT;
        } else if (!strncmp(request->req_ext, "cgi", 3)) {
            int cgiin[2], cgiout[2];
            pid_t cgi_pid;
            char c;
            cgi_rec cr = { 0 };

            if (pipe(cgiin) == -1 || pipe(cgiout) == -1) {
                fprintf(stderr, "create pipe error\n");
                goto RESPOND_INTERNEL_ERROR;
            }

            if (cgi_preprocess(&cr, c_addr_ptr, request) == -1) {
                fprintf(stderr, "cgi preprocessing error\n");
                goto RESPOND_INTERNEL_ERROR;
            }
#ifdef DEBUG
            int debug_i = 0;
            while (cr.rmv[debug_i] != NULL)
                printf("[c] %s\n", cr.rmv[debug_i++]);
            if (cr.rmb != NULL)
                printf("[C] %s\n", cr.rmb);
#endif

            /* create a new process to execute cgi program */
            if ((cgi_pid = fork()) < 0) {
                fprintf(stderr, "fork cgi process error\n");
                goto RESPOND_INTERNEL_ERROR;
            }

            if (cgi_pid == 0) {
                char **cgi_argv;

                cgi_argv = malloc(sizeof(char *) * 2);
                cgi_argv[0] = path;
                cgi_argv[1] = NULL;

                // close unused fd
                close(cgiin[1]);
                close(cgiout[0]);

                // redirect fd
                dup2(cgiin[0], STDIN_FILENO);
                dup2(cgiout[1], STDOUT_FILENO);

                // close unused fd
                close(cgiin[0]);
                close(cgiout[1]);

                /* execute cgi program */
                execve(path, cgi_argv, cr.rmv);

                /* error, should never reach */
                exit(1);
            }
            // close unused fd
            close(cgiin[0]);
            close(cgiout[1]);

            // write POST data to cgi program via pipe
            if (cr.rmb != NULL)
                write(cgiin[1], cr.rmb, strlen(cr.rmb));

            http_send_headers(cfd, 
                              request, 
                              proto, 
                              STATE_200, 
                              NULL,
                              NULL);
            /* cgi program can write some custom header */

            while (read(cgiout[0], &c, 1) > 0) {
                write(cfd, &c, 1);
            }

            // close unused fd
            close(cgiin[1]);
            close(cgiout[0]);

            *status = 200;
            goto RESPOND_EXIT;
        } else {
            FILE *fp;
            char *fcontent;

            fp = fopen(path, "rb");
            if (fp == NULL)
                goto NOT_FOUND;

            fseek(fp, 0, SEEK_END);
            fsize = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            s_fsize = malloc(0x70);
            snprintf(s_fsize, 0x70, "%ld", fsize);

            /* read path */
            fcontent = malloc(fsize + 1);
            fread(fcontent, 1, fsize, fp);
            fclose(fp);

            fcontent[fsize] = 0;

            http_send_headers(cfd, 
                              request, 
                              proto, 
                              STATE_200, 
                              CONTENT_TYPE_PLAIN,
                              s_fsize);
            WRITE(CR_LF);
            write(cfd, fcontent, fsize);

            free(s_fsize);
            free(fcontent);
            *status = 200;
            goto RESPOND_EXIT;
        }
    } else {
        /* path doesn't exist */
NOT_FOUND:
        s_fsize = malloc(0x70);
        snprintf(s_fsize, 0x70, "%ld", strlen(BODY_404));

        http_send_headers(cfd, 
                          request, 
                          proto, 
                          STATE_404, 
                          CONTENT_TYPE_HTML,
                          s_fsize);
        WRITE(CR_LF);
        WRITE(BODY_404);

        free(s_fsize);
        *status = 404;
        goto RESPOND_EXIT;
    }

    if (0) {
RESPOND_INTERNEL_ERROR:
        s_fsize = malloc(0x70);
        snprintf(s_fsize, 0x70, "%ld", strlen(BODY_500));

        http_send_headers(cfd, 
                          request, 
                          proto, 
                          STATE_500, 
                          CONTENT_TYPE_HTML,
                          s_fsize);
        WRITE(CR_LF);
        WRITE(BODY_500);

        free(s_fsize);
        *status = 500;
        goto RESPOND_EXIT;
    }

RESPOND_EXIT:
    free(path);
    return;
}
