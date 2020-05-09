#include "general_ssl.h"

void init_openssl(void)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

void set_cert_pkey(SSL_CTX *ctx, config_rec *c)
{
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, c->cert_file,
                SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "certificate error");
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, c->pkey_file,
                SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "private key error");
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_check_private_key(ctx) <= 0) {
        fprintf(stderr, "certificate and key mismatch");
        exit(EXIT_FAILURE);
    }
}

void set_ca_cert(SSL_CTX *ctx, config_rec *c)
{
    if (SSL_CTX_load_verify_locations(ctx, c->CA_cert_file, NULL) <= 0) {
        fprintf(stderr, "CA certificate error");
        exit(EXIT_FAILURE);
    } 

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    SSL_CTX_set_verify_depth(ctx, 1);
}

int check_cert(SSL *ssl)
{
    X509 *peer_cert;
    char *str;

    /* check a certificate was presented */
    peer_cert = SSL_get_peer_certificate(ssl);

    if (peer_cert == NULL) {
        return -1;
    }

    str = X509_NAME_oneline(X509_get_subject_name(peer_cert), 0, 0);
    if (str != NULL) {
        printf("  subject: %s\n", str);
        free(str);
    } else {
        goto NO_INFO_ERROR;
    }

    str = X509_NAME_oneline(X509_get_issuer_name(peer_cert), 0, 0);
    if (str != NULL) {
        printf("  issuer: %s\n", str);
        free(str);
    } else {
        goto NO_INFO_ERROR;
    }
    
    X509_free(peer_cert);

    /* check the result of certificate verification */
    if (SSL_get_verify_result(ssl) != X509_V_OK) {
        return -3;
    }

    return 0;

NO_INFO_ERROR:
    X509_free(peer_cert);
    return -2;
}

SSL *create_ssl_connect(int sock, SSL_CTX *ctx, int server)
{
    SSL *ssl;
    int i;

    /* Initialize ssl */
    if ((ssl = SSL_new(ctx)) == NULL) {
        fprintf(stderr, "ssl error");
        exit(EXIT_FAILURE);
    }
    SSL_set_fd(ssl, sock);

    /* SSL Handshake, key exchange */
    if (server) {
        if ((i = SSL_accept(ssl)) <= 0) {
            if (SSL_get_verify_result(ssl) != X509_V_OK) {
                fprintf(stderr, "client verification failed\n");
            } else {
                fprintf(stderr, "unexpected accept error\n");
            }

            exit(EXIT_FAILURE);
        }
    } else {
        if ((i = SSL_connect(ssl)) <= 0) {
            if (SSL_get_verify_result(ssl) != X509_V_OK) {
                fprintf(stderr, "server verification failed\n");
            } else {
                fprintf(stderr, "unexpected accept error\n");
            }

            exit(EXIT_FAILURE);
        }
    }

    return ssl;
}

