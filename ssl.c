#include "ssl.h"
#include "error.h"
#include "common.h"


SSL_CTX* create_ssl_context(int is_server) {
    const SSL_METHOD* method;
    SSL_CTX* ctx;

    if (is_server) {
        method = SSLv23_server_method();
    } else {
        method = SSLv23_client_method();
    }

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        log_error(ERROR_SSL, "Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    return ctx;
}

ErrorCode configure_ssl_context(SSL_CTX* ssl_ctx, const char* cert_file, const char* key_file) {

    // Load the server's certificate and key
    if (SSL_CTX_use_certificate_file(ssl_ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
        log_error(ERROR_SSL, "Failed to load certificate");
        return ERROR_SSL;
    }

    if (SSL_CTX_use_PrivateKey_file(ssl_ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        log_error(ERROR_SSL, "Failed to load private key");
        return ERROR_SSL;
    }

    // Verify that the private key matches the certificate
    if (!SSL_CTX_check_private_key(ssl_ctx)) {
        log_error(ERROR_SSL, "Private key does not match the certificate public key");
        return ERROR_SSL;
    }

    return ERROR_NONE;
}