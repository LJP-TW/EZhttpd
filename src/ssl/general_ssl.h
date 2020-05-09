#ifndef GENERAL_SSL_H
#define GENERAL_SSL_H

#include <openssl/ssl.h>
#include <openssl/err.h>

struct _config_rec {
    char *CA_cert_file;
    char *cert_file;
    char *pkey_file;
    /* DON'T CHANGE order of above config */
};

typedef struct _config_rec config_rec;

extern void init_openssl(void);

extern void set_cert_pkey(SSL_CTX *ctx, config_rec *c);

extern void set_ca_cert(SSL_CTX *ctx, config_rec *c);

extern int check_cert(SSL *ssl);

extern SSL *create_ssl_connect(int sock, SSL_CTX *ctx, int server);

#endif
