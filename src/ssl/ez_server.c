#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "ssl/general_ssl.h"
#include "ez_server.h"

void server_handle_parameters(int argc, char **argv, server_parameters *sp)
{
    int cmd_opt = 0;
    int tmp;

    while (1) {
        cmd_opt = getopt(argc, argv, "i:p:f:h");

        if (cmd_opt == -1) {
            break;
        }

        switch (cmd_opt) {
            case 'i':
                tmp = strlen(optarg) + 1;
                sp->ip = malloc(tmp);
                memset(sp->ip, 0, tmp);
                strcat(sp->ip, optarg);
                break;
            case 'p':
                sp->port = atoi(optarg);
                break;
            case 'f':
                tmp = strlen(optarg) + 1;
                sp->config_file = malloc(tmp);
                memset(sp->config_file, 0, tmp);
                strcat(sp->config_file, optarg);
                break;
            case 'h':
                printf("Usage: " SERVER_BIN_NAME " [OPTION]...\n");
                printf("\n");
                printf("  -i\tip\n");
                printf("  -p\tport\n");
                printf("  -f\tconfig file\n");
                printf("  -h\thelp\n");
                exit(EXIT_SUCCESS);
                break;

            case '?':
            default:
                fprintf(stderr, "Try '" SERVER_BIN_NAME " -h' for more information.\n");
                exit(EXIT_FAILURE);
        }
    }

    if (sp->ip == NULL) {
        fprintf(stderr, SERVER_BIN_NAME ": need to set parameter -i\n");
        exit(EXIT_FAILURE);
    } else if (sp->port == 0) {
        fprintf(stderr, SERVER_BIN_NAME ": need to set parameter -p\n");
        exit(EXIT_FAILURE);
    } else if (sp->config_file == NULL) {
        fprintf(stderr, SERVER_BIN_NAME ": need to set parameter -f\n");
        exit(EXIT_FAILURE);
    }
}

void server_parse_config_file(server_parameters *sp, server_config_rec *sc)
{
    FILE *fp;
    char name[0x80];
    char val[0x80];

    if ((fp = fopen(sp->config_file, "r")) == NULL) {
        fprintf(stderr, SERVER_BIN_NAME ": cannot read sc file\n");
        exit(EXIT_FAILURE);
    }

    while (fscanf(fp, "%127[^=]=%127[^\n]%*c", name, val) == 2) {
        // TODO: Verify client certificate 
        // if (strncmp("CA_CERT", name, 7) == 0) {
        //     sc->CA_cert_file = malloc(0x80);
        //     memset(sc->CA_cert_file, 0, 0x80);
        //     strcat(sc->CA_cert_file, val);
        // }
        if (strncmp("ENABLE_SSL", name, 10) == 0) {
            sc->enable_ssl = val[0] == '1' ? 1 : 0;
        } else if (strncmp("SERVER_CERT", name, 11) == 0) {
            sc->server_cert_file = malloc(0x80);
            memset(sc->server_cert_file, 0, 0x80);
            strcat(sc->server_cert_file, val);
        } else if (strncmp("SERVER_PKEY", name, 11) == 0) {
            sc->server_pkey_file = malloc(0x80);
            memset(sc->server_pkey_file, 0, 0x80);
            strcat(sc->server_pkey_file, val);
        } else if (strncmp("WEB_ROOT", name, 8) == 0) {
            sc->web_root = malloc(0x80);
            memset(sc->web_root, 0, 0x80);
            strcat(sc->web_root, val);
        } else if (strncmp("LIST_FILES", name, 10) == 0) {
            sc->list_files = val[0] == '1' ? 1 : 0;
        } else if (strncmp("DOWNLOADABLE", name, 12) == 0) {
            sc->downloadable = val[0] == '1' ? 1 : 0;
        }
    }
}

SSL_CTX *server_create_context(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        fprintf(stderr, "unable to create SSL context");
        exit(EXIT_FAILURE);
    }
    
    return ctx;
}

void server_set_cert_pkey(SSL_CTX *ctx, server_config_rec *sc)
{
    set_cert_pkey(ctx, (config_rec *)sc);
}

void server_set_ca_cert(SSL_CTX *ctx, server_config_rec *sc)
{
    set_ca_cert(ctx, (config_rec *)sc);
}

int server_create_socket(server_parameters *sp)
{
    int s;
    int TRUE = 1;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(sp->port);
    addr.sin_addr.s_addr = inet_addr(sp->ip); 

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        fprintf(stderr, "socket error");
        exit(EXIT_FAILURE);
    }

    /* set SO_REUSEADDR, check out this link for detail:
     *   https://stackoverflow.com/questions/10619952/how-to-completely-destroy-a-socket-connection-in-c 
     */
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &TRUE, sizeof(TRUE)) == -1) {
        fprintf(stderr, "setsockopt error\n");
        exit(EXIT_FAILURE);
    }

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(s, 100) < 0) {
        fprintf(stderr, "listen error");
        exit(EXIT_FAILURE);
    }

    return s;
}

void server_init(int argc, 
                 char **argv, 
                 server_parameters **sp, 
                 server_config_rec **sc,
                 SSL_CTX **ctx)
{
    *sp = malloc(sizeof(server_parameters));
    *sc = malloc(sizeof(server_config_rec));

    server_handle_parameters(argc, argv, *sp);
    server_parse_config_file(*sp, *sc);

    if ((*sc)->enable_ssl) {
        init_openssl();

        *ctx = server_create_context();
        server_set_cert_pkey(*ctx, *sc);

        // TODO: Verify client certificate
        // server_set_ca_cert(*ctx, *sc);
    }
}

SSL *server_create_ssl_connect(int sock, SSL_CTX *ctx)
{
    return create_ssl_connect(sock, ctx, 1);
}

