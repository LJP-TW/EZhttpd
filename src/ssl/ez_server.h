#ifndef EZ_SERVER_C
#define EZ_SERVER_C

#include <openssl/ssl.h>

#define SERVER_BIN_NAME "ez_rsserver"

struct _server_parameters {
    char *ip;
    char *config_file;
    unsigned int port;
};

typedef struct _server_parameters server_parameters;

/* The config variables at config file */
struct _server_config_rec {
    char *CA_cert_file;
    char *server_cert_file;
    char *server_pkey_file;
    /* DON'T CHANGE order of above config */
    char *web_root;
    unsigned char enable_ssl:1;
};

typedef struct _server_config_rec server_config_rec;

void server_handle_parameters(int argc, char **argv, server_parameters *sp);
void server_parse_config_file(server_parameters *sp, server_config_rec *sc);
SSL_CTX *server_create_context(void);
void server_set_cert_pkey(SSL_CTX *ctx, server_config_rec *sc);
void server_set_ca_cert(SSL_CTX *ctx, server_config_rec *sc);
int server_create_socket(server_parameters *sp);

void server_init(int argc, 
                 char **argv, 
                 server_parameters **sp, 
                 server_config_rec **sc,
                 SSL_CTX **ctx);

SSL *server_create_ssl_connect(int sock, SSL_CTX *ctx);

#endif
