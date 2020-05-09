#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/prctl.h>

#include "http/http_request.h"
#include "http/http_parser.h"
#include "http/http_respond.h"
#include "cgi/cgi_preprocess.h"
#include "debug/debug.h"
#include "ssl/ez_server.h"
#include "ssl/general_ssl.h"

void respond(union conn, struct sockaddr_in *, server_config_rec *sc);
void ez_log(int, http_request_rec *, struct sockaddr_in *);

int main(int argc, char **argv)
{ 
    SSL_CTX *ctx = NULL;
    server_parameters *sp = NULL;
    server_config_rec *sc = NULL;
    int sfd;

    server_init(argc, argv, &sp, &sc, &ctx);

    sfd = server_create_socket(sp);

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
        } else if (pid == 0) {
            /* when parent exit, childs also exit */
            union conn client;

            prctl(PR_SET_PDEATHSIG, SIGHUP);
            close(sfd);

            if (sc->enable_ssl) {
                client.ssl = server_create_ssl_connect(cfd, ctx);
            } else {
                client.cfd = cfd;
            }

            respond(client, &c_addr, sc);
            exit(0);
        }

        close(cfd);
    }

    close(sfd);
    SSL_CTX_free(ctx);
}

void respond(union conn client, struct sockaddr_in *c_addr_ptr, server_config_rec *sc)
{
    int ret, status;
    http_request_rec request = { 0 };

    http_init_request(&request);

    while (1) {
        ret = http_handle_request(client, &request, sc->enable_ssl);

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

        http_respond(client, 
                     &request, 
                     sc, 
                     c_addr_ptr,
                     &status);

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
