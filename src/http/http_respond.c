#include <string.h>
#include <unistd.h>

#include "http_respond.h"
#include "cgi/cgi_preprocess.h"
#include "ez_list.h"

#define WRITE(fd, X) http_write(fd, X, strlen(X))
#define WRITE_CONTENT_LENGTH(fd, SIZE) \
    WRITE(fd, CONTENT_LENGTH); \
    WRITE(fd, SIZE); \
    WRITE(fd, CR_LF);
#define HTTP_KEEP_ALIVE(TIMEOUT, MAX) \
    "Keep-Alive: timeout=" TIMEOUT ", max=" MAX "\r\n"

#define MAX_BUF_LEN 0x1000

/* States for counting content-length */
enum CGI_STATES {
    CS_INIT,
    CS_R,
    CS_RN,
    CS_RNR,
    CS_RNRN
};

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

typedef ssize_t (*write_method_proto) (long long __fd, const void *__buf, size_t __n);
write_method_proto http_write;

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
void http_send_headers(long long fd,
                       http_request_rec *request, 
                       const char *proto, 
                       const char *state,
                       const char *content_type,
                       const char *content_length)
{
    WRITE(fd, proto);
    WRITE(fd, state);
    WRITE(fd, SERVER);

    if (content_type)
        WRITE(fd, content_type);

    if (request->keep_alive)
        WRITE(fd, CONNECTION_KEEP_ALIVE);

    if (content_length)
        WRITE_CONTENT_LENGTH(fd, content_length);
}

void http_respond(union conn client, 
                  http_request_rec *request, 
                  server_config_rec *sc,
                  struct sockaddr_in *c_addr_ptr,
                  int *status)
{
    const char *proto;
    char *path;
    char *s_fsize;
    long long fd;
    long fsize;

    if (sc->enable_ssl) {
        http_write = (write_method_proto) SSL_write;
        fd = (long long)client.ssl;
    }
    else {
        http_write = (write_method_proto) write;
        fd = client.cfd;
    }

    proto = http_protos[request->proto]; 
    path = complete_path(request, sc->web_root);

    if (request->req_ext == (char *)-1) {
        /* path is a directory */
        /* TODO: list directory */
        s_fsize = malloc(0x70);
        snprintf(s_fsize, 0x70, "%ld", strlen(BODY_404));

        http_send_headers(fd, 
                          request, 
                          proto, 
                          STATE_404, 
                          CONTENT_TYPE_HTML,
                          s_fsize);
        WRITE(fd, CR_LF);
        WRITE(fd, BODY_404);

        free(s_fsize);
        *status = 404;
        goto RESPOND_EXIT;
    }

    if (access(path, F_OK) != -1) {
        if (!request->req_ext) {
            goto OTHER_EXT;
        } else if (!strncmp(request->req_ext, "html", 4)) {
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

            http_send_headers(fd, 
                              request, 
                              proto, 
                              STATE_200, 
                              CONTENT_TYPE_HTML,
                              s_fsize);
            WRITE(fd, CR_LF);
            http_write(fd, fcontent, fsize);

            free(s_fsize);
            free(fcontent);
            *status = 200;
            goto RESPOND_EXIT;
        } else if (!strncmp(request->req_ext, "cgi", 3)) {
            char *now_data;
            int cgiin[2], cgiout[2], now_len, cgi_state;
            long content_len;
            pid_t cgi_pid;
            cgi_rec cr = { 0 };
            ez_list *cgi_bufs, *now_cgi_bufs;
            char c[1];

            cgi_state = CS_INIT;
            content_len = now_len = 0;

            now_cgi_bufs = cgi_bufs = malloc(sizeof(ez_list));
            now_data = cgi_bufs->data = malloc(sizeof(char) * MAX_BUF_LEN);
            cgi_bufs->next = NULL;

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

            /* write the output of cgi program to buf first
             * then calculate content-length and send this header
             * for keep-alive mechanism to work
             */
            while (read(cgiout[0], c, 1) > 0) {
                now_data[now_len] = c[0]; 

                switch (cgi_state) {
                case CS_INIT:
                    if (c[0] == '\r')
                        cgi_state = CS_R;
                    break;
                case CS_R:
                    if (c[0] == '\n')
                        cgi_state = CS_RN;
                    else
                        cgi_state = CS_INIT;
                    break;
                case CS_RN:
                    if (c[0] == '\r')
                        cgi_state = CS_RNR;
                    else
                        cgi_state = CS_INIT;
                    break;
                case CS_RNR:
                    if (c[0] == '\n')
                        cgi_state = CS_RNRN;
                    else
                        cgi_state = CS_INIT;
                    break;
                case CS_RNRN:
                    ++content_len;
                    break;
                }

                ++now_len;
                if (now_len == MAX_BUF_LEN) {
                    /* create next node of singly linked list */
                    now_cgi_bufs->next = malloc(sizeof(ez_list));

                    /* goto next node */
                    now_cgi_bufs = now_cgi_bufs->next;
                    
                    /* initial this node */
                    now_data = now_cgi_bufs->data = malloc(MAX_BUF_LEN);
                    now_cgi_bufs->next = NULL;

                    /* re-count now_len */
                    now_len = 0;
                }
            }

            /* cgi program should write '\r\n\r\n' itself
             * for now, server doesn't check this problem
             */

            s_fsize = malloc(0x70);
            snprintf(s_fsize, 0x70, "%ld", content_len);

            http_send_headers(fd, 
                              request, 
                              proto, 
                              STATE_200, 
                              NULL,
                              s_fsize);

            /* cgi program can write some custom header */

            now_cgi_bufs = cgi_bufs;
            while (1) {
                if (now_cgi_bufs->next) {
                    http_write(fd, now_cgi_bufs->data, MAX_BUF_LEN);     
                    now_cgi_bufs = now_cgi_bufs->next;
                } else {
                    http_write(fd, now_cgi_bufs->data, now_len);
                    break;
                }
            }

            // close unused fd
            close(cgiin[1]);
            close(cgiout[0]);

            free(s_fsize);
            ez_free_list(cgi_bufs);
            cgi_rec_destroy(&cr);
            *status = 200;
            goto RESPOND_EXIT;
        } else {
            FILE *fp;
            char *fcontent;

OTHER_EXT:
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

            http_send_headers(fd, 
                              request, 
                              proto, 
                              STATE_200, 
                              CONTENT_TYPE_PLAIN,
                              s_fsize);
            WRITE(fd, CR_LF);
            http_write(fd, fcontent, fsize);

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

        http_send_headers(fd, 
                          request, 
                          proto, 
                          STATE_404, 
                          CONTENT_TYPE_HTML,
                          s_fsize);
        WRITE(fd, CR_LF);
        WRITE(fd, BODY_404);

        free(s_fsize);
        *status = 404;
        goto RESPOND_EXIT;
    }

    if (0) {
RESPOND_INTERNEL_ERROR:
        s_fsize = malloc(0x70);
        snprintf(s_fsize, 0x70, "%ld", strlen(BODY_500));

        http_send_headers(fd, 
                          request, 
                          proto, 
                          STATE_500, 
                          CONTENT_TYPE_HTML,
                          s_fsize);
        WRITE(fd, CR_LF);
        WRITE(fd, BODY_500);

        free(s_fsize);
        *status = 500;
        goto RESPOND_EXIT;
    }

RESPOND_EXIT:
    free(path);
    return;
}
