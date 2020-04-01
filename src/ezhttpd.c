#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>

#include "http_parser.h"
#include "cgi_preprocess.h"
#include "debug.h"

#define MAX_CONNECTION 100000
#define MAX_POST_PAR_NUM 100

void respond(int, struct sockaddr_in *);
void ez_log(int, http_request_rec *, struct sockaddr_in *);

char *root;

int main(int argc, char **argv)
{ 
    struct sockaddr_in s_addr;
    in_addr_t ip;
    int sfd, len;
    int t = 1;

    if (argc < 4) {
        printf("usage: ./ezhttpd [ipv4] [port] [root]");
        exit(EXIT_FAILURE);
    }

    len = strlen(argv[3]);
    if (argv[3][len - 1] != '/') {
        root = malloc(len + 2);
        memset(root, 0, len + 2);
        strcat(root, argv[3]);
        root[len] = '/';
    } else {
        root = strdup(argv[3]);
    }

    sfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sfd == -1) {
        fprintf(stderr, "socket error\n");
        exit(EXIT_FAILURE);
    }

    /* set SO_REUSEADDR, check out this link for detail:
     *   https://stackoverflow.com/questions/10619952/how-to-completely-destroy-a-socket-connection-in-c 
     */
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int)) == -1) {
        fprintf(stderr, "setsockopt error\n");
        exit(EXIT_FAILURE);
    }

    bzero(&s_addr, sizeof(s_addr));
    s_addr.sin_family = PF_INET;
    s_addr.sin_addr.s_addr = ip = inet_addr(argv[1]);
    s_addr.sin_port = htons(atoi(argv[2]));

    if (ip == INADDR_NONE) {
        fprintf(stderr, "inet_addr error: ip %s is invalid\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    
    if (bind(sfd, (struct sockaddr *)&s_addr, sizeof(s_addr)) == -1) {
        fprintf(stderr, "bind error\n");
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, MAX_CONNECTION)) {
        fprintf(stderr, "listen error\n");
        exit(EXIT_FAILURE);
    }

    /* when child process exit, just let it be deleted, 
     * preventing zombie process */
    signal(SIGCHLD, SIG_IGN);

    while (1) {
        struct sockaddr_in c_addr;
        socklen_t c_addr_len;
        pid_t pid;
        int cfd;

        c_addr_len = sizeof(c_addr);
        cfd = accept(sfd, (struct sockaddr *)&c_addr, &c_addr_len);

        if (cfd < 0) {
            fprintf(stderr, "accept error\n");
            continue;
        }

        /* create a new process to handle request */
        if ((pid = fork()) < 0) {
            fprintf(stderr, "fork error\n");
            close(cfd);
            continue;
        }
        else if (pid == 0) {
            /* when parent exit, childs also exit */
            prctl(PR_SET_PDEATHSIG, SIGHUP);
            close(sfd);

            respond(cfd, &c_addr);

            close(cfd);

            exit(0);
        } else {
            close(cfd);
        }
    }
}

void respond(int cfd, struct sockaddr_in *c_addr_ptr)
{
    const char *proto;
    char *rbuf, *path;
    int ret, len, status;
    http_request_rec request = { 0 };

    rbuf = malloc(65535);

    ret = recv(cfd, rbuf, 65535, 0);

    if (ret == -1) {
        fprintf(stderr, "recv error\n");
        goto RESPOND_EARLY_EXIT;
    } else if (ret == 0) {
        fprintf(stderr, "socket peer has performed an orderly shutdown\n");
        goto RESPOND_EARLY_EXIT;
    }

    rbuf[ret] = '\0';

    /* handle request header */
    if(http_parser(rbuf, &request) == -1) {
        fprintf(stderr, "error during parsing the request\n");
        goto RESPOND_EARLY_EXIT;
    }
#ifdef DEBUG
    DEBUG_REQUEST_INFO;
#endif
    
    len = strlen(root) + strlen(request.req_path);
    path = malloc(len + 1);
    memset(path, 0, len + 1);
    strcat(path, root);
    strcat(path, request.req_path);

    proto = http_protos[request.proto];

#ifdef DEBUG
    printf("[>] path: %s\n", path);
#endif

    if (request.req_ext == (char *)-1) {
        /* path is a directory */
        /* TODO: list directory */
        write(cfd, proto, strlen(proto));
        write(cfd, " 404 Not Found\r\n", 16);
        write(cfd, "Server: ezhttpd\r\n", 17);
        write(cfd, "\r\n", 2);
        write(cfd, "<h1>Not Found, 88888</h1>", 25);

        status = 404;
        goto RESPOND_EXIT;
    }

    if (access(path, F_OK) != -1) {
        if (!strncmp(request.req_ext, "html", 4)) {
            FILE *fp;
            char *fcontent;
            long fsize;

            fp = fopen(path, "rb");
            if (fp == NULL)
                goto NOT_FOUND;

            fseek(fp, 0, SEEK_END);
            fsize = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            write(cfd, proto, strlen(proto));
            write(cfd, " 200 OK\r\n", 9);
            write(cfd, "Server: ezhttpd\r\n", 17);
            write(cfd, "\r\n", 2);

            /* read path */
            fcontent = malloc(fsize + 1);
            fread(fcontent, 1, fsize, fp);
            fclose(fp);

            fcontent[fsize] = 0;

            write(cfd, fcontent, fsize);

            free(fcontent);
            status = 200;
        } else if (!strncmp(request.req_ext, "cgi", 3)) {
            int cgiin[2], cgiout[2];
            pid_t cgi_pid;
            char c;
            cgi_rec cr = { 0 };

            if (pipe(cgiin) == -1 || pipe(cgiout) == -1) {
                fprintf(stderr, "create pipe error\n");
                goto RESPOND_INTERNEL_ERROR;
            }

            if (cgi_preprocess(&cr, c_addr_ptr, &request) == -1) {
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

            write(cfd, proto, strlen(proto));
            write(cfd, " 200 OK\r\n", 9);
            write(cfd, "Server: ezhttpd\r\n", 17);
            /* cgi program can write some custom header */

            while (read(cgiout[0], &c, 1) > 0) {
                write(cfd, &c, 1);
            }

            // close unused fd
            close(cgiin[1]);
            close(cgiout[0]);

            status = 200;
        } else {
            FILE *fp;
            char *fcontent;
            long fsize;
            char *s_fsize;

            fp = fopen(path, "rb");
            if (fp == NULL)
                goto NOT_FOUND;

            fseek(fp, 0, SEEK_END);
            fsize = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            s_fsize = malloc(0x70);
            snprintf(s_fsize, 0x70, "%ld", fsize);

            write(cfd, proto, strlen(proto));
            write(cfd, " 200 OK\r\n", 9);
            write(cfd, "Server: ezhttpd\r\n", 17);
            write(cfd, "Content-Type: text/plain\r\n", 26);
            write(cfd, "Content-Length: ", 16);
            write(cfd, s_fsize, strlen(s_fsize));
            write(cfd, "\r\n", 2);
            write(cfd, "\r\n", 2);

            /* read path */
            fcontent = malloc(fsize + 1);
            fread(fcontent, 1, fsize, fp);
            fclose(fp);

            fcontent[fsize] = 0;

            write(cfd, fcontent, fsize);

            free(fcontent);
            status = 200;
        }
    } else {
NOT_FOUND:
        /* path doesn't exist */
        write(cfd, proto, strlen(proto));
        write(cfd, " 404 Not Found\r\n", 16);
        write(cfd, "Server: ezhttpd\r\n", 17);
        write(cfd, "\r\n", 2);
        write(cfd, "<h1>Not Found, 88888</h1>", 25);
        status = 404;
    }

    if (0) {
RESPOND_INTERNEL_ERROR:
        write(cfd, proto, strlen(proto));
        write(cfd, " 500 Internal Server Error\r\n", 28);
        write(cfd, "Server: ezhttpd\r\n", 17);
        write(cfd, "\r\n", 2);
        write(cfd, "<h1>Internal Server Error</h1>", 30);
        status = 500;
    }

RESPOND_EXIT:
    /* log */
    ez_log(status, &request, c_addr_ptr);

    http_request_rec_destroy(&request);

    free(path);

RESPOND_EARLY_EXIT:
    free(rbuf);
}

inline 
void ez_log(int status,
            http_request_rec *hrr,
            struct sockaddr_in *c_addr_ptr)
{
    const char *method, *proto;
    char *ip_str, *url;
    int color;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    ip_str = inet_ntoa(c_addr_ptr->sin_addr);

    /* switch log color code */
    switch(status / 100) {
        case 2:
            color = 32; /* green */
            break;
        case 4:
        case 5:
            color = 31; /* red */
            break;
        default:
            color = 33; /* yellow */
    }

    method = http_methods[hrr->method];
    proto = http_protos[hrr->proto];
    url = hrr->req_path;

    printf("[%d-%02d-%02d %02d:%02d:%02d] ", 
            tm.tm_year + 1900, 
            tm.tm_mon + 1, 
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec);

    printf("\033[0;%dm%s:%d [%d]: %s %s %s\033[0m\n", 
            color,
            ip_str, 
            c_addr_ptr->sin_port, 
            status,
            method, 
            url, 
            proto);
}
