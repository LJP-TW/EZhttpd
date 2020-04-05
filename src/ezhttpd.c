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

#include "http_request.h"
#include "http_parser.h"
#include "http_respond.h"
#include "cgi_preprocess.h"
#include "debug.h"

#define MAX_CONNECTION 100000

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
    int ret, status;
    http_request_rec request = { 0 };

    http_init_request(&request);

    while (1) {
        ret = http_handle_request(cfd, &request);

        switch (ret) {
        case -1:
#ifdef DEBUG
            printf("[O] timeout\n");
            goto RESPOND_EXIT;
#endif
        case -2:
        case -3:
            goto RESPOND_EXIT;
        }

#ifdef DEBUG
        DEBUG_REQUEST_INFO;
#endif

        http_respond(cfd, &request, root, c_addr_ptr, &status);

        /* log */
        ez_log(status, &request, c_addr_ptr);

        /* handle keep-alive mechanism */
        if (!request.keep_alive || !request.keep_alive_amount)
            break;
        
        --request.keep_alive_amount;

        http_destroy_request(&request);
    }

RESPOND_EXIT:
#ifdef DEBUG
    printf("[T] close TCP connection\n");
#endif
    http_destroy_request(&request);

    return;
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
            ntohs(c_addr_ptr->sin_port), 
            status,
            method, 
            url, 
            proto);
}
