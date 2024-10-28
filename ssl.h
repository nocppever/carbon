#ifndef SSL_H
#define SSL_H

#include "common.h"
#include "error.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

// Function declarations for SSL context
SSL_CTX* create_ssl_context(int is_server);
ErrorCode configure_ssl_context(SSL_CTX* ssl_ctx, const char* cert_file, const char* key_file);


static inline ErrorCode init_openssl(void) {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    return ERROR_NONE;
}

static inline void cleanup_openssl(void) {
    EVP_cleanup();
}

static inline ErrorCode connect_to_server(Config* config, SSL_CTX* ssl_ctx, 
                                        SSL** ssl, int* sock) {
    struct sockaddr_in server_addr;
    
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock < 0) {
        return ERROR_NETWORK;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config->server_port);
    if (inet_pton(AF_INET, config->server_ip, &server_addr.sin_addr) <= 0) {
        closesocket(*sock);
        return ERROR_NETWORK;
    }
    
    if (connect(*sock, (struct sockaddr*)&server_addr, 
                sizeof(server_addr)) < 0) {
        closesocket(*sock);
        return ERROR_NETWORK;
    }
    
    *ssl = SSL_new(ssl_ctx);
    SSL_set_fd(*ssl, *sock);
    
    if (SSL_connect(*ssl) <= 0) {
        SSL_free(*ssl);
        closesocket(*sock);
        return ERROR_SSL;
    }
    
    return ERROR_NONE;
}

#endif // SSL_H